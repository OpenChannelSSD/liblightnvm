/* Management tests */

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_name[DISK_NAME_LEN] = "nvme0n1";

int init_suite1(void)
{
	return 0;
}

int clean_suite1(void)
{
	return 0;
}

void test_DEV_OPEN_CLOSE(void)
{
	NVM_DEV dev;

	dev = nvm_dev_open(nvm_dev_name);
	CU_ASSERT_PTR_NOT_NULL(dev);

	nvm_dev_close(dev);
        CU_ASSERT_PTR_NOT_NULL(dev);
	// still dangling...
}

void test_DEV_OPEN_CLOSE_N(void)
{
	NVM_DEV dev;
	const int n = 10;
	int i;

	for(i=0; i<n; ++i) {
		dev = nvm_dev_open(nvm_dev_name);
		CU_ASSERT_PTR_NOT_NULL(dev);
	}

	for(i=0; i<n; ++i) {
		nvm_dev_close(dev);
		CU_ASSERT_PTR_NOT_NULL(dev);
		// still dangling...
	}
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
	pSuite = CU_add_suite("nvm_dev*", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if (
	(NULL == CU_add_test(pSuite, "nvm_dev_[open|close]", test_DEV_OPEN_CLOSE)) ||
	(NULL == CU_add_test(pSuite, "nvm_dev_[open|close] n", test_DEV_OPEN_CLOSE_N)) ||
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

