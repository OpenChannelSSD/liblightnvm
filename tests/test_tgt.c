/* Management tests */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_name[DISK_NAME_LEN] = "nvme0n1";
static char nvm_tgt_name[DISK_NAME_LEN] = "nvm_tgt_tst";
static char nvm_tgt_type_name[NVM_TTYPE_NAME_MAX] = "dflash";

int init_suite1(void)
{
	return 0;
}

int clean_suite1(void)
{
	return 0;
}

void test_TGT_INFO_NEW_FREE(void)
{
	NVM_TGT_INFO info;

	info = nvm_tgt_info_new();
	CU_ASSERT_PTR_NOT_NULL(info);

	nvm_tgt_info_free(&info);
	CU_ASSERT_PTR_NULL(info);
}

void test_TGT_INFO_FILL(void)
{
	NVM_TGT_INFO info;
	int ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type_name, nvm_dev_name,
                                  0, 0);
	CU_ASSERT(0 == ret);

	info = nvm_tgt_info_new();
	CU_ASSERT_PTR_NOT_NULL(info);

	ret = nvm_tgt_info_fill(info, nvm_tgt_name);
	CU_ASSERT(0 == ret);

	nvm_tgt_info_free(&info);
	CU_ASSERT_PTR_NULL(info);

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0 == ret);
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

	if (argc > 2) {
		if (strlen(argv[1]) > DISK_NAME_LEN) {
			printf("len(tgt_name) > %d\n", DISK_NAME_LEN - 1);
			return -1;
		}
		strcpy(nvm_tgt_name, argv[2]);
	}

	if (argc > 3) {
		if (strlen(argv[3]) > NVM_TTYPE_NAME_MAX) {
			printf("len(tgt_type_name) > %d\n", NVM_TTYPE_NAME_MAX - 1);
		}
		strcpy(nvm_tgt_type_name, argv[3]);
	}

	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("nvm_tgt*", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if (
	(NULL == CU_add_test(pSuite, "nvm_tgt_info[new|free]", test_TGT_INFO_NEW_FREE)) ||
	(NULL == CU_add_test(pSuite, "nvm_tgt_info_fill", test_TGT_INFO_FILL)) ||
	0
	)
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
