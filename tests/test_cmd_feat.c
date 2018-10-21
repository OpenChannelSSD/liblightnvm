#include "test_intf.c"

static void _test_assert_dulbe_enabled()
{
	union nvm_nvme_feat feat = { 0 };
	int rc;

	rc = nvm_cmd_gfeat(dev, NVM_NVME_FEAT_ERROR_RECOVERY, &feat, NULL);
	CU_ASSERT(rc == 0);

	CU_ASSERT(feat.error_recovery.dulbe == 0x1);
}

static void _test_assert_dulbe_disabled()
{
	union nvm_nvme_feat feat = { 0 };
	int rc;

	rc = nvm_cmd_gfeat(dev, NVM_NVME_FEAT_ERROR_RECOVERY, &feat, NULL);
	CU_ASSERT(rc == 0);

	CU_ASSERT(feat.error_recovery.dulbe == 0x0);
}

static void test_set_dulbe_enable()
{
	union nvm_nvme_feat feat = { .error_recovery.dulbe = 1, };
	int rc;

	rc = nvm_cmd_sfeat(dev, NVM_NVME_FEAT_ERROR_RECOVERY, &feat, NULL);
	CU_ASSERT(rc == 0);

	_test_assert_dulbe_enabled();
}

static void test_set_dulbe_disable()
{
	union nvm_nvme_feat feat = { .error_recovery.dulbe = 0, };
	int rc;

	rc = nvm_cmd_sfeat(dev, NVM_NVME_FEAT_ERROR_RECOVERY, &feat, NULL);
	CU_ASSERT(rc == 0);

	_test_assert_dulbe_disabled();
}

static void test_get()
{
	union nvm_nvme_feat feat = { 0 };
	int rc;

	rc = nvm_cmd_gfeat(dev, NVM_NVME_FEAT_ERROR_RECOVERY, &feat, NULL);
	CU_ASSERT(rc == 0);
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_feat",
					argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "feat: {feature: ERROR_RECOVERY; get}", test_get))
		goto out;
	if (!CU_add_test(pSuite, "feat: {feature: ERROR_RECOVERY; attribute: DULBE; set: DISABLE}", test_set_dulbe_disable))
		goto out;
	if (!CU_add_test(pSuite, "feat: {feature: ERROR_RECOVERY; attribute: DULBE; set: ENABLE}", test_set_dulbe_enable))
		goto out;
	if (!CU_add_test(pSuite, "[CLEANUP] feat: {feature: ERROR_RECOVERY; attribute: DULBE; set: DISABLE}", test_set_dulbe_disable))
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
