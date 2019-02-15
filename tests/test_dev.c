#include "test_intf.c"

// Verify that the device can be opened
void test_DEV_OPEN_CLOSE(void)
{
	struct nvm_dev *dev;

	dev = nvm_dev_open(NVM_DEV_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dev);

	nvm_dev_close(dev);
}

// Verify that the device can be opened more than once
void test_DEV_OPEN_CLOSE_N(void)
{
	struct nvm_dev *dev[] = {0,0,0,0,0,0,0,0,0,0};
	int ndev = sizeof(dev) / sizeof(dev[0]);

	for (int i = 0; i < ndev; ++i) {
		dev[i] = nvm_dev_open(NVM_DEV_PATH);
		CU_ASSERT_PTR_NOT_NULL(dev[i]);
		if (!dev[i])
			break;

		nvm_dev_close(dev[i]);
	}
}

// Verify that the device can be opened more than once at the same time
void test_DEV_OPEN_CLOSE_MULTI_N(void)
{
	struct nvm_dev *dev[] = {0,0,0,0,0,0,0,0,0,0};
	int ndev = sizeof(dev) / sizeof(dev[0]);

	for (int i = 0; i < ndev; ++i) {
		dev[i] = nvm_dev_open(NVM_DEV_PATH);
		CU_ASSERT_PTR_NOT_NULL(dev[i]);
		if (!dev[i])
			break;
	}

	for (int i = 0; i < ndev; ++i) {
		if (!dev[i])
			break;
		nvm_dev_close(dev[i]);
	}
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create_nosetup("nvm_dev_*", argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "nvm_dev_[open|close]", test_DEV_OPEN_CLOSE))
		goto out;
	if (!CU_add_test(pSuite, "nvm_dev_[open|close] n", test_DEV_OPEN_CLOSE_N))
		goto out;
	if (!CU_add_test(pSuite, "nvm_dev_[open|close] multi-n", test_DEV_OPEN_CLOSE_MULTI_N))
		goto out;

	switch(RMODE) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(RMODE);
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
