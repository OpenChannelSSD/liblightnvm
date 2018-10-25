#include "test_util.h"
#include "test_intf.c"

static void test_compliance_scalar_write_exceed_mdts()
{
	SPEC_20_ONLY

	struct nvm_addr addr;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	char *buf = nvm_buf_alloc(DEV, (MAX_SCALAR_LBAS + WS_MIN) * SECTOR_SIZE, 0);
	CU_ASSERT_PTR_NOT_NULL(buf);

	// this should exceed typical MDTS; so expect an error
	nvm_test_scalar_write_oneshot_err(addr, MAX_SCALAR_LBAS + WS_MIN, buf, NVME_ERR_INVALID_FIELD);

	nvm_buf_free(DEV, buf);
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_compliance",
					argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "compliance: {state: FREE; mode: SCALAR; write, exceed MDTS}", test_compliance_scalar_write_exceed_mdts))
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
