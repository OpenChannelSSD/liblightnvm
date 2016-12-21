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

//	return nvm_addr_check(lun_addr, geo);
	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);

	return 0;
}

// Get BBT, change it, then persist it using nvm_bbt_set and verify it
// using nvm_bbt_get
void test_BBT_GET_SET_GET(void)
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
	}

	nvm_bbt_free(bbt_act);
	nvm_bbt_free(bbt_exp);

	return;
}

// Mark some blocks using nvm_bbt_mark, then use get to verify that the
// change persisted
void test_BBT_MARK_GET(void)
{
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
	(NULL == CU_add_test(pSuite, "BBT get -> set -> get", test_BBT_GET_SET_GET)) ||
	(NULL == CU_add_test(pSuite, "BBT mark -> get", test_BBT_MARK_GET)) ||
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
