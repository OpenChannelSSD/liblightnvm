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
	NVM_BBT_HMRK
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
		CU_FAIL("FAILED: Unexpected value of bbt->nblks");
		return;
	}

	// Verify that it contains valid states
	for (int i = 0; i < bbt->nblks; ++i) {
		switch(bbt->blks[i]) {
			case NVM_BBT_FREE:
			case NVM_BBT_BAD:
			case NVM_BBT_GBAD:
			case NVM_BBT_HMRK:
			case NVM_BBT_DMRK:
				break;
			default:
				nvm_bbt_free(bbt);
				CU_FAIL("FAILED: Invalid value bbt->blks[i]");
				return;
		}
	}

	nvm_bbt_free(bbt);

	return;
}

//
// Test that we can set bbt using `nvm_bbt_mark`
//
// @note
// The test changes the state of NVM_NADDR_MAX number of blocks nstates times.
// Verified is done by retrieving the bbt and comparing actual state with
// expected state.
//
// @warn
// This will alter the state of the bad-block-table on the device. It will most
// likely leave the bad-block-table in a different state than it was in before
// running this test
//
void _test_BBT_MARK_NADDR(unsigned int naddr_pr_call)
{
	const int nblks = NVM_NADDR_MAX;
	
	const int nvblks = nblks / geo->nplanes;
	const int vblk_ofz = 4;
	const int vblk_skip = (geo->nblocks - vblk_ofz) / nvblks;

	struct nvm_addr addrs[nblks];

	for (int i = 0; i < nblks; i += geo->nplanes) {	// Construct addresses
		int vblk = vblk_ofz + (i / geo->nplanes) * vblk_skip;
		for (int pl = 0; pl < geo->nplanes; ++pl) {
			addrs[i + pl].ppa = lun_addr.ppa;
			addrs[i + pl].g.blk = vblk;
			addrs[i + pl].g.pl = pl;
		}
	}
	
	for (int i = 0; i < nblks; ++i) {	// Verify constructed addrs
		if (nvm_addr_check(addrs[i], geo)) {
			CU_FAIL("FAILED: Constructing addresses");
			return;
		}
	}

	for (int st_i = 0; st_i < nstates; ++st_i) {
		struct nvm_bbt *bbt;

		// Persist the results using "NVM_NADDR_MAX / naddr_pr_call"
		// number of calls to nvm_bbt_mark
		for (int ofz = 0; ofz < nblks; ofz += naddr_pr_call) {
			nvm_bbt_mark(dev, &addrs[ofz], naddr_pr_call,
				     states[st_i], NULL);
		}

		bbt = nvm_bbt_get(dev, lun_addr, NULL);
		CU_ASSERT_PTR_NOT_NULL(bbt);
		if (!bbt) {
			CU_FAIL("FAILED: Retrieving bbt for verification");
			return;
		}

		for (int i = 0; i < nblks; ++i) {	// Verify state
			int idx = addrs[i].g.blk * geo->nplanes + addrs[i].g.pl;
			CU_ASSERT_EQUAL(bbt->blks[idx], states[st_i]);
			if (bbt->blks[idx] != states[st_i]) {
				CU_FAIL("FAILED: Unexpected value of bbt");
				break;
			}
		}

		nvm_bbt_free(bbt);
	}
	
	return;
}

void test_BBT_MARK_NADDR_MAX(void)
{
	_test_BBT_MARK_NADDR(NVM_NADDR_MAX);
}

void test_BBT_MARK_NADDR_MAX2(void)
{
	_test_BBT_MARK_NADDR(NVM_NADDR_MAX / 2);
}

void test_BBT_MARK_NADDR_MAX4(void)
{
	_test_BBT_MARK_NADDR(NVM_NADDR_MAX / 4);
}

void test_BBT_MARK_NADDR_1(void)
{
	_test_BBT_MARK_NADDR(1);
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
		CU_FAIL("FAILED: nvm_bbt_get -- prior to nvm_bbt_set");
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
				bbt_exp->blks[idx] = NVM_BBT_HMRK;
		}
	}

	res = nvm_bbt_set(dev, bbt_exp, NULL);	// Persist changes
	if (res < 0) {
		CU_FAIL("FAILED: nvm_bbt_set");
		nvm_bbt_free(bbt_exp);
		return;
	}

	bbt_act = nvm_bbt_get(dev, lun_addr, NULL);
	if (!bbt_act) {
		CU_FAIL("FAILED: nvm_bbt_get -- after nvm_bbt_set")
		nvm_bbt_free(bbt_exp);
		return;
	}

	CU_ASSERT_EQUAL(bbt_exp->nblks, bbt_act->nblks);	// Verify bbt

	for (int blk = 0; blk < bbt_act->nblks; ++blk) {
		CU_ASSERT_EQUAL(bbt_exp->blks[blk], bbt_act->blks[blk]);
		if (bbt_exp->blks[blk] != bbt_act->blks[blk]) {
			CU_FAIL("FAILED: Unexpected value of bbt");
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
	(NULL == CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX)", test_BBT_MARK_NADDR_MAX)) ||
	(NULL == CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX/2)", test_BBT_MARK_NADDR_MAX2)) ||
	(NULL == CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX/4)", test_BBT_MARK_NADDR_MAX4)) ||
	(NULL == CU_add_test(pSuite, "nvm_bbt_mark (NADDR=1)", test_BBT_MARK_NADDR_1)) ||
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
