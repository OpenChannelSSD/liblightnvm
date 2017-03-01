#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

void test_DEV_OPEN_CLOSE(void)
{
	struct nvm_dev *dev;

	dev = nvm_dev_open(nvm_dev_path);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dev);

	nvm_dev_close(dev);
}

void test_DEV_OPEN_CLOSE_N(void)
{
	const int n = 10;
	struct nvm_dev *dev[10];

	for (int i = 0; i < n; ++i) {
		dev[i] = nvm_dev_open(nvm_dev_path);
		CU_ASSERT_PTR_NOT_NULL(dev[i]);
		if (!dev[i])
			break;
	}

	for (int i = 0; i < n; ++i) {
		if (!dev[i])
			break;
		nvm_dev_close(dev[i]);
	}
}

void test_DEV_OPENF_SYSFS_CLOSE(void)
{
	struct nvm_dev *dev;

	dev = nvm_dev_openf(nvm_dev_path, NVM_BE_SYSFS);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dev);

	CU_ASSERT_EQUAL(nvm_dev_get_be_id(dev), NVM_BE_SYSFS);

	nvm_dev_close(dev);
}

void test_DEV_OPENF_IOCTL_CLOSE(void)
{
	struct nvm_dev *dev;

	dev = nvm_dev_openf(nvm_dev_path, NVM_BE_IOCTL);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dev);

	CU_ASSERT_EQUAL(nvm_dev_get_be_id(dev), NVM_BE_IOCTL);

	nvm_dev_close(dev);
}

int main(int argc, char **argv)
{
	if (argc > 1) {
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("len(dev_path) > %d\n", NVM_DEV_PATH_LEN - 1);
			return -1;
		}
		strcpy(nvm_dev_path, argv[1]);
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
	(NULL == CU_add_test(pSuite, "nvm_dev_[open|close]", test_DEV_OPEN_CLOSE)) ||
	(NULL == CU_add_test(pSuite, "nvm_dev_[open|close] n", test_DEV_OPEN_CLOSE_N)) ||
	(NULL == CU_add_test(pSuite, "nvm_dev_[openf(sysfs)|close] ", test_DEV_OPENF_SYSFS_CLOSE)) ||
	(NULL == CU_add_test(pSuite, "nvm_dev_[openf(ioctl)|close] ", test_DEV_OPENF_IOCTL_CLOSE)) ||
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

