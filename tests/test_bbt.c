#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int channel = 0;
static int lun = 0;

static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr lun_addr;

// States used when verifying that bad-block-table can be set
static enum nvm_bbt_state states[] = {
	NVM_BBT_FREE,
	NVM_BBT_HBAD
};
static int nstates = sizeof(states) / sizeof(states[0]);

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
	}
	geo = nvm_dev_attr_geo(dev);

	lun_addr.ppa = 0;
	lun_addr.g.ch = channel;
	lun_addr.g.lun = lun;

	return nvm_addr_check(lun_addr, geo);
}

int teardown(void)
{
	nvm_dev_close(dev);

	return 0;
}

/**
 * Test that we can get a valid bad-block-table from a device
 */
void test_BBT_GET(void)
{
	struct nvm_bbt *bbt;

	bbt = nvm_bbt_get(dev, lun_addr, NULL);
	// Verify that we can call it
	CU_ASSERT_PTR_NOT_NULL(bbt);
	if (!bbt)
		return;

	// Verify that it contains the expected number of blocks
	CU_ASSERT_EQUAL(bbt->nblks, geo->nplanes * geo->nblocks);
	if (bbt->nblks != geo->nplanes * geo->nblocks) {
		nvm_bbt_free(bbt);
		CU_FAIL("Unexpected value of bbt->nblks");
		return;
	}

	// Verify that it contains valid states
	for (int i = 0; i < bbt->nblks; ++i) {
		switch(bbt->blks[i]) {
			case NVM_BBT_FREE:
			case NVM_BBT_BAD:
			case NVM_BBT_GBAD:
			case NVM_BBT_HBAD:
			case NVM_BBT_DBAD:
				CU_PASS("Valid bbt->blks[i]");
				break;
			default:
				nvm_bbt_free(bbt);
				CU_FAIL("Invalid value bbt->blks[i]");
				return;
		}
	}

	nvm_bbt_free(bbt);

	return;
}

//
// Test that we can set bbt using `nvm_bbt_mark` with multiple addresses in a
// single command
//
// @note
// We do not want to change state of all blocks so we spread them out and change
// only "NVM_ADDR_MAX / geo->nplanes" number of blocks
//
// @warn
// This will alter the state of the bad-block-table on the device.
// It will most likely leave the bad-block-table in a different state than
// it was in before running this test
//
void test_BBT_MARK_NADDR(void)
{
	const int nblks = NVM_NADDR_MAX / (geo->nplanes);	// Spanning plns
	struct nvm_addr addrs[nblks];
	const int ofz = 4;
	const int skip = geo->nblocks / (nblks + ofz);	// Spread them out

	CU_ASSERT_FATAL(nblks);
	CU_ASSERT_FATAL(ofz);
	CU_ASSERT_FATAL((ofz + nblks) < geo->nblocks);

	for (int state_i = 0; state_i < nstates; ++state_i) {
		struct nvm_bbt *bbt;
		int res;

		// Construct addresses to change state for
		for (int i = 0; i < nblks; i += geo->nplanes) {
			const int blk = i * skip + ofz;

			for (int pl = 0; pl < geo->nplanes; ++pl) {
				addrs[i + pl].ppa = lun_addr.ppa;
				addrs[i + pl].g.blk = blk;
				addrs[i + pl].g.pl = pl;
			}
		}
		for (int i = 0; i < nblks; ++i) {
			if (nvm_addr_check(addrs[i], geo)) {
				CU_FAIL("Invalid addresses");
				return;
			}
		}

		// Persist bbt state for nblks
		res = nvm_bbt_mark(dev, addrs, nblks, states[state_i], NULL);
		CU_ASSERT(!res);
		if (!res) {
			CU_FAIL("FAILED: nvm_bbt_mark");
			return;
		}

		// Retrieve persisted state
		bbt = nvm_bbt_get(dev, lun_addr, NULL);
		CU_ASSERT_PTR_NOT_NULL(bbt);
		if (!bbt) {
			CU_FAIL("FAILED: nvm_bbt_get");
		}

		// Verify that the state is as expected
		for (int i = 0; i < nblks; ++i) {
			int idx = addrs[i].g.blk * geo->nplanes + addrs[i].g.pl;
			CU_ASSERT_EQUAL(bbt->blks[idx], states[state_i]);
			if (bbt->blks[idx] != states[state_i]) {
				CU_FAIL("Unexpected bad-block-table state");
				break;
			}
		}
		nvm_bbt_free(bbt);
	}
	
	return;
}

//
// Test that we can set bbt using `nvm_bbt_mark` with one address and multiple
// commands
//
// @note
// We do not want to change state of all blocks so we spread them out and change
// only "NVM_ADDR_MAX / geo->nplanes" number of blocks
//
// @warn
// This will alter the state of the bad-block-table on the device.
// It will most likely leave the bad-block-table in a different state than
// it was in before running this test
//
void test_BBT_MARK_1ADDR(void)
{
	const int nblks = NVM_NADDR_MAX / (geo->nplanes);	// Spanning plns
	struct nvm_addr addrs[nblks];
	const int ofz = 4;
	const int skip = geo->nblocks / (nblks + ofz);	// Spread them out

	CU_ASSERT(nblks);
	CU_ASSERT(ofz);
	CU_ASSERT((ofz + nblks) < geo->nblocks);

	for (int state_i = 0; state_i < nstates; ++state_i) {
		struct nvm_bbt *bbt;
		int res;

		// Construct addresses to change state for
		for (int i = 0; i < nblks; i += geo->nplanes) {
			const int blk = i * skip + ofz;

			for (int pl = 0; pl < geo->nplanes; ++pl) {
				addrs[i + pl].ppa = lun_addr.ppa;
				addrs[i + pl].g.blk = blk;
				addrs[i + pl].g.pl = pl;

				// Persist bbt state for nblks
				res = nvm_bbt_mark(dev, &addrs[i + pl], 1,
						   states[state_i], NULL);
				CU_ASSERT(!res);
			}
		}

		// Retrieve persisted state
		bbt = nvm_bbt_get(dev, lun_addr, NULL);
		CU_ASSERT_PTR_NOT_NULL(bbt);

		// Verify that the state is as expected
		for (int i = 0; i < nblks; ++i) {
			int idx = addrs[i].g.blk * geo->nplanes + addrs[i].g.pl;
			CU_ASSERT_EQUAL(bbt->blks[idx], states[state_i]);
			if (bbt->blks[idx] != states[state_i]) {
				CU_FAIL("Unexpected bad-block-table state");
				break;
			}
		}
		nvm_bbt_free(bbt);
	}
	
	return;
}

// Test that we can set bbt using `nvm_bbt_set`
//
// @warn
// This will alter the state of the bad-block-table on the device.
// It will most likely leave the bad-block-table in a different state than
// it was in before running this test
//
void test_BBT_SET(void)
{
	struct nvm_bbt *bbt_exp, *bbt_act;
	int res;

	bbt_exp = nvm_bbt_get(dev, lun_addr, NULL);
	if (!bbt_exp) {
		CU_FAIL("nvm_bbt_get -- prior to nvm_bbt_set");
		return;
	}

	// Check that we have the expected number of blocks
	CU_ASSERT_EQUAL(bbt_exp->nblks, geo->nplanes * geo->nblocks);

	for (int i = 1; i < 5; ++i) {		// Flip state of four blocks
		for (int pl = 0; pl < geo->nplanes; ++pl) {
			int idx = (geo->nblocks/(i*2)) + pl;
			if (bbt_exp->blks[idx])
				bbt_exp->blks[idx] = NVM_BBT_FREE;
			else
				bbt_exp->blks[idx] = NVM_BBT_HBAD;
		}
	}

	res = nvm_bbt_set(dev, bbt_exp, NULL);	// Persist changes
	if (res < 0) {
		CU_FAIL("nvm_bbt_set");
		nvm_bbt_free(bbt_exp);
		return;
	}

	bbt_act = nvm_bbt_get(dev, lun_addr, NULL);
	if (!bbt_act) {
		CU_FAIL("nvm_bbt_get -- after nvm_bbt_set")
		nvm_bbt_free(bbt_exp);
		return;
	}

	CU_ASSERT_EQUAL(bbt_exp->nblks, bbt_act->nblks);	// Verify bbt

	for (int blk = 0; blk < bbt_act->nblks; ++blk) {
		CU_ASSERT_EQUAL(bbt_exp->blks[blk], bbt_act->blks[blk]);
		if (bbt_exp->blks[blk] != bbt_act->blks[blk]) {
			CU_FAIL("Unexpected bad-block-table state");
			break;
		}
	}

	nvm_bbt_free(bbt_act);
	nvm_bbt_free(bbt_exp);

	return;
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 4:
		lun = atoi(argv[3]);
	case 3:
		channel = atoi(argv[2]);
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return 1;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}
	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_addr_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_bbt_get", test_BBT_GET)) ||
	(NULL == CU_add_test(pSuite, "nvm_bbt_mark (naddr)", test_BBT_MARK_NADDR)) ||
	(NULL == CU_add_test(pSuite, "nvm_bbt_mark (1addr)", test_BBT_MARK_1ADDR)) ||
	(NULL == CU_add_test(pSuite, "nvm_bbt_set", test_BBT_SET)) ||
	0)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_NORMAL);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
