/**
 * This tests attempts to determine whether or not the nvm_dev_mark function
 * works as expected. It does so by marking all blocks bad, then tries to _get
 * one. Which is then expected to fail, since all blocks are bad.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_name[DISK_NAME_LEN] = "nvme0n1";

void TEST_DEV_MARK(void)
{
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_VBLOCK vblk;

	int vblocks_total;			/* Total number of vblocks  */
	int i, ch, lun;

	int mark_total;				/* Calls to mark */
	int mark_failed;			/* Failed calls to mark */
	
	vblk = nvm_vblock_new();		/* Allocate a vblock */
	if (!vblk) {
		CU_FAIL();
		return;
	}

	dev = nvm_dev_open(nvm_dev_name);	/* Open device */
	if (!dev) {
		nvm_vblock_free(&vblk);
		CU_FAIL();
		return;
	}

	geo = nvm_dev_attr_geo(dev);		/* Get geometry */

	vblocks_total = geo.nluns * geo.nblocks;

	mark_total = 0;
	mark_failed = 0;
	for (i = 0; i < vblocks_total; ++i) {
		NVM_ADDR addr;
		int err;

		addr.g.ch = i % geo.nchannels;	/* Setup block address */
		addr.g.lun = i % geo.nluns;
		addr.g.blk = i % geo.nblocks;

		nvm_addr_pr(addr);

		err = nvm_dev_mark(dev, addr, 0x1);
		mark_total++;
		if (err) {
			mark_failed++;
		}
	}

	nvm_geo_pr(geo);
	printf("vblocks_total(%d), mark_total(%d) / mark_failed(%d)\n",
		vblocks_total, mark_total, mark_failed);

	// Now try to get a block via each channel/lun, they should all fail
	for (ch = 0; ch < geo.nchannels; ch++) {
		for (lun = 0; lun < geo.nluns; ++lun) {
			int err = nvm_vblock_gets(vblk, dev, ch, lun);
			if (!err) {
				printf("What? No error?\n");
				nvm_vblock_pr(vblk);
			}
			nvm_vblock_put(vblk);
		}
	}

	nvm_dev_close(dev);
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		if (strlen(argv[1]) > DISK_NAME_LEN) {
			printf("len(dev_name) > %d\n", DISK_NAME_LEN - 1);
			return -1;
		}
		strcpy(nvm_dev_name, argv[1]);
	}

	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("nvm_dev*", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if (
	(NULL == CU_add_test(pSuite, "nvm_dev_mark", TEST_DEV_MARK)) ||
	0
	)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_SILENT);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}

