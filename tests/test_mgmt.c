/* Management tests */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_name[DISK_NAME_LEN];
static char nvm_tgt_name[DISK_NAME_LEN] = "nvm_mgmt_tst";
static char nvm_tgt_type_name[NVM_TTYPE_NAME_MAX] = "dflash";

int init_suite1(void)
{
	return 0;
}

int clean_suite1(void)
{
	return 0;
}

void test_CREATE_REMOVE(void)
{
	int ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type_name, nvm_dev_name,
                                  0, 0);
	CU_ASSERT(0 == ret);

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0 == ret);
}

int main(int argc, char **argv)
{
	if (argc != 4) {
		printf("Usage: %s nvme_dev tgt_name tgt_type_name\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(nvme_dev) > %d\n", DISK_NAME_LEN - 1);
	}

	if (strlen(argv[2]) > DISK_NAME_LEN) {
		printf("len(tgt_name) > %d\n", DISK_NAME_LEN - 1);
	}

	if (strlen(argv[3]) > NVM_TTYPE_NAME_MAX) {
		printf("len(tgt_type_name) > %d\n", NVM_TTYPE_NAME_MAX - 1);
	}

	strcpy(nvm_dev_name, argv[1]);
	strcpy(nvm_tgt_name, argv[2]);
	strcpy(nvm_tgt_type_name, argv[3]);

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_mgmt*", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_mgmt_tgt_[create|remove]", test_CREATE_REMOVE)) ||
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
