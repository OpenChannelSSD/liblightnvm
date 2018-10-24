#include "test_intf.c"

void test_DEV_OPEN_CLOSE(void)
{
	struct nvm_dev *dev;

	dev = nvm_dev_open(NVM_DEV_PATH);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dev);

	nvm_dev_close(dev);
}

void test_DEV_OPEN_CLOSE_N(void)
{
	const int n = 10;
	struct nvm_dev *dev[10];

	for (int i = 0; i < n; ++i) {
		dev[i] = nvm_dev_open(NVM_DEV_PATH);
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
