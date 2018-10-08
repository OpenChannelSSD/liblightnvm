#include "test_intf.c"

static void set_error_recovery(bool on)
{
	union nvm_nvme_feat feat = { 0 };

	if (on) {
		feat.error_recovery.dulbe = 1;
	}

	if (nvm_cmd_sfeat(dev, NVM_FEAT_ERROR_RECOVERY, &feat, NULL)) {
		CU_FAIL("nvm_cmd_sfeat");
		return;
	}
}

static void test_GETSET_FEATURE()
{
	union nvm_nvme_feat feat = { 0 };
	if (nvm_cmd_gfeat(dev, NVM_FEAT_ERROR_RECOVERY, &feat, NULL)) {
		CU_FAIL("nvm_cmd_gfeat");
		return;
	}

	if (feat.error_recovery.dulbe) {
		set_error_recovery(false);

		if (nvm_cmd_gfeat(dev, NVM_FEAT_ERROR_RECOVERY, &feat, NULL)) {
			CU_FAIL("nvm_cmd_gfeat");
			return;
		}

		CU_ASSERT(feat.error_recovery.dulbe == 0);

		set_error_recovery(true);
	} else {
		set_error_recovery(true);

		if (nvm_cmd_gfeat(dev, NVM_FEAT_ERROR_RECOVERY, &feat, NULL)) {
			CU_FAIL("nvm_cmd_gfeat");
			return;
		}

		CU_ASSERT(feat.error_recovery.dulbe == 1);

		set_error_recovery(false);
	}

	CU_PASS("Success");
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_feat",
					argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "GET/SET FEAT", test_GETSET_FEATURE))
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
