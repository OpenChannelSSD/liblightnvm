#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_name[DISK_NAME_LEN] = "nvme0n1";
static char nvm_tgt_type[NVM_TTYPE_NAME_MAX] = "dflash";
static char nvm_tgt_name[DISK_NAME_LEN] = "nvm_vblock_tst";

int init_suite1(void)
{
	return 0;
}

int clean_suite1(void)
{
	return 0;
}
void __test_VBLOCK_WRITE_READ_N(int iterations, int npage_io)
{
	NVM_DEV dev;
	NVM_FPAGE fpage;
	uint32_t sec_size, pln_pg_size;

	NVM_TGT tgt;
	NVM_VBLOCK vblock;
	int i, j, ret;
						/* Create target */
	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev_name, 0, 0);
	CU_ASSERT(0==ret);

	dev = nvm_dev_open(nvm_dev_name);		/* Open device */
	CU_ASSERT_PTR_NOT_NULL(dev);

	fpage = nvm_dev_get_fpage(dev);		/* Get geometry */
	sec_size = nvm_fpage_get_sec_size(fpage);
	pln_pg_size = nvm_fpage_get_pln_pg_size(fpage);

	tgt = nvm_tgt_open(nvm_tgt_name, 0x0);	/* Open target */
	CU_ASSERT_PTR_NOT_NULL(tgt);

	for(i=0; i<iterations; ++i) {
		int read, written;
		char *wbuf = NULL;
		char *rbuf = NULL;
							/* Allocate buffers */
		ret = posix_memalign((void**)&wbuf, sec_size, pln_pg_size);
		CU_ASSERT(0==ret)
		strcpy(wbuf, "Hello World of NVM");

		ret = posix_memalign((void**)&rbuf, sec_size, pln_pg_size);
		CU_ASSERT(0==ret);

		vblock = nvm_vblock_new();		/* Allocate vblock */
		CU_ASSERT_PTR_NOT_NULL(vblock);

		ret = nvm_vblock_get(vblock, tgt);
		CU_ASSERT(0==ret)
		for(j=0; j<npage_io; ++j) {
			written = nvm_vblock_write(vblock, wbuf, 1, 0, 0x0);
			CU_ASSERT(1==written)

			read = nvm_vblock_read(vblock, rbuf, 1, 0, 0x0);
			CU_ASSERT(1==read);

			CU_ASSERT_STRING_EQUAL(wbuf, rbuf);
		}

		ret = nvm_vblock_put(vblock);	/* Release vblock from tgt */
		CU_ASSERT(0==ret);

		free(wbuf);
		free(rbuf);

		nvm_vblock_free(&vblock);		/* De-allocate vblock */
	}

	nvm_tgt_close(tgt);				/* Close the target */

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0==ret);
}

void test_VBLOCK_WRITE_READ_01(void)
{
	__test_VBLOCK_WRITE_READ_N(1, 1);
}

void test_VBLOCK_WRITE_READ_02(void)
{
	__test_VBLOCK_WRITE_READ_N(1000, 1);
}

void test_VBLOCK_WRITE_READ_03(void)
{
	__test_VBLOCK_WRITE_READ_N(1, 1000);
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

	pSuite = CU_add_suite("nvm_vblock*", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 1", test_VBLOCK_WRITE_READ_01)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 2", test_VBLOCK_WRITE_READ_02)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 3", test_VBLOCK_WRITE_READ_03)) ||
	0)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
