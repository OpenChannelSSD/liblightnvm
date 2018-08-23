#include "test_intf.c"

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
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_dev_*",
					argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "nvm_dev_[open|close]", test_DEV_OPEN_CLOSE))
		goto out;
	if (!CU_add_test(pSuite, "nvm_dev_[open|close] n", test_DEV_OPEN_CLOSE_N))
		goto out;
	if (!CU_add_test(pSuite, "nvm_dev_[openf(ioctl)|close] ", test_DEV_OPENF_IOCTL_CLOSE))
		goto out;

	switch(rmode) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(rmode);
		CU_basic_run_tests();
		break;
	}

out:
	err = CU_get_error() || \
	      CU_get_number_of_suites_failed() || \
	      CU_get_number_of_tests_failed() || \
	      CU_get_number_of_failures();

	CU_cleanup_registry();

	return err;
}
