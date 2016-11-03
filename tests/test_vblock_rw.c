#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_name[DISK_NAME_LEN] = "nvme0n1";

void __TEST_VBLOCK_PWRITE_READ_N(int iterations, int npage_io)
{
	NVM_DEV dev;
	NVM_GEO geo;

	NVM_VBLK vblock;
	int i, j;
	ssize_t err;

	dev = nvm_dev_open(nvm_dev_name);	/* Open device */
	CU_ASSERT_PTR_NOT_NULL(dev);
	
	geo = nvm_dev_attr_geo(dev);

	for(i=0; i<iterations; ++i) {
		char *wbuf;
		char *rbuf;
						/* Allocate buffers */
		wbuf = nvm_buf_alloc(geo, geo.vpg_nbytes);
		CU_ASSERT_PTR_NOT_NULL(wbuf);
		strcpy(wbuf, "Hello World of NVM");

		rbuf = nvm_buf_alloc(geo, geo.vpg_nbytes);
		CU_ASSERT_PTR_NOT_NULL(rbuf);

		vblock = nvm_vblk_new();	/* Allocate vblock */
		CU_ASSERT_PTR_NOT_NULL(vblock);

		err = nvm_vblk_get(vblock, dev);
		CU_ASSERT(!err);
		for(j=0; j < npage_io; ++j) {
			err = nvm_vblk_pwrite(vblock, wbuf, 0);
			CU_ASSERT(!err);

			err = nvm_vblk_pread(vblock, rbuf, 0);
			CU_ASSERT(!err);

			CU_ASSERT_STRING_EQUAL(wbuf, rbuf);
		}

		err = nvm_vblk_put(vblock);	/* Release vblock from dev */
		CU_ASSERT(!err);

		free(wbuf);
		free(rbuf);

		nvm_vblk_free(&vblock);	/* De-allocate vblock */
	}

	nvm_dev_close(dev);			/* Close the device */
}

void test_vblock_pwrite_READ_01(void)
{
	__TEST_VBLOCK_PWRITE_READ_N(1, 1);
}

void test_vblock_pwrite_READ_02(void)
{
	__TEST_VBLOCK_PWRITE_READ_N(1000, 1);
}

void test_vblock_pwrite_READ_03(void)
{
	__TEST_VBLOCK_PWRITE_READ_N(1, 1000);
}

int main(int argc, char **argv)
{
	if (argc > 1) {
                if (strlen(argv[1]) > DISK_NAME_LEN) {
                        printf("Argument nvm_dev can be maximum %d characters\n",
                                                                DISK_NAME_LEN - 1);
                }
		strcpy(nvm_dev_name, argv[1]);
	}

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_vblk*", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 1", test_vblock_pwrite_READ_01)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 2", test_vblock_pwrite_READ_02)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 3", test_vblock_pwrite_READ_03)) ||
	0)
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
