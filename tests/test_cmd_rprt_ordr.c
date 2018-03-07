#include "test_intf.c"

/**
 * Retrieve all entries in the chunk information log-page
 *
 * Verifies:
 *
 *  - Number of entries in the log-page complies with spec. 2.0
 *  - Ordering of the entries in the log-page complies with spec. 2.0
 *  - That chunk states are valid values as defined by spec. 2.0
 */

void test_CMD_RPRT_ORDR(void)
{
	struct nvm_spec_rprt *rprt = NULL;

	size_t tchunks = geo->l.npugrp * geo->l.npunit * geo->l.nchunk;

	rprt = nvm_cmd_rprt(dev, NULL, 0x0, NULL);
	CU_ASSERT_PTR_NOT_NULL(rprt);
	if (!rprt)
		goto out;

	CU_ASSERT(tchunks == rprt->ndescr);
	if (tchunks != rprt->ndescr)
		goto out;

	for (size_t i = 0; i < tchunks; ++i) {
		const struct nvm_addr addr = {
		.l.sectr = 0,
		.l.chunk = i % geo->l.nchunk,
		.l.punit = (i / geo->l.nchunk) % geo->l.npunit,
		.l.pugrp = ((i / geo->l.nchunk) / geo->l.npunit) % geo->l.npugrp
		};

		const uint32_t addr_dev = nvm_addr_gen2dev(dev, addr);

		struct nvm_spec_rprt_descr *descr = &rprt[i];

		switch (descr->cs) {	// Verify the chunk_state value
		case NVM_CHUNK_STATE_FREE:
		case NVM_CHUNK_STATE_CLOSED:
		case NVM_CHUNK_STATE_OPEN_:
		case NVM_CHUNK_STATE_OFFLINE:
			break;
		default:
			CU_FAIL("rprt.chunk_descr.cs has non-compliant value");
			break;
		}

		CU_ASSERT(descr->addr == addr_dev);
	}

out:
	nvm_buf_free(rprt);
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_cmd_rprt ORDR", argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "nvm_cmd_rprt ORDR", test_CMD_RPRT_ORDR))
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
