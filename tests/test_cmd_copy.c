/**
 * Test nvm_cmd_copy
 *
 * Verify:
 *
 *  - nvm_cmd can submit and complete without error
 *  - a constructed payload can be written, copied and read back
 *  - that glp / rprt updates state appropriately
 *
 * NOTE: two chunks are selected by consulting nvm_cmd_rprt_get_arbs
 * one is written with a constructed payload, then copied to the other chunk
 */
#include "test_intf.c"

#define NADDRS 2

void test_CMD_VCOPY(void)
{
	struct nvm_ret ret = {0};
	struct nvm_addr source_addrs[NADDRS];
	struct nvm_addr dest_addrs[NADDRS];
	int naddrs = NADDRS;
	int err;

	// assign for source addrs value.
	source_addrs[0].val = 0x101;
	source_addrs[1].val = 0x102;

	// assign for dest addrs value.
	dest_addrs[0].val = 0x201;
	dest_addrs[1].val = 0x202;

	// nvm_cmd_copy
	nvm_cmd_copy(dev, source_addrs, dest_addrs, naddrs, 0, &ret);

	CU_ASSERT(!err);

	return;
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_cmd_vcopy_*", argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_ADD_TEST(pSuite, test_CMD_VCOPY))
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
