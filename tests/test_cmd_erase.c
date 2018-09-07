#include "test_intf.c"

static void erase_s20(int use_metadata, int erase_mode)
{
	struct nvm_ret ret;
	struct nvm_addr chunk_addr = { .val = 0 };
	ssize_t res;

	// get an abitrary free chunk
	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &chunk_addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	struct nvm_addr lun_addr = { .val = 0 };
	struct nvm_spec_rprt_descr *descr = NULL;
	struct nvm_spec_rprt *rprt = NULL;

	if (use_metadata) {
		lun_addr.l.pugrp = chunk_addr.l.pugrp;
		lun_addr.l.punit = chunk_addr.l.punit;

		// get report for all chunks in lun
		if (NULL == (rprt = nvm_cmd_rprt(dev, &lun_addr, 0, NULL))) {
			CU_FAIL("nvm_cmd_rprt failed");
			return;
		}

		descr = nvm_buf_alloc(geo, sizeof(struct nvm_spec_rprt_descr));
	}

	// erase the chunk
	res = nvm_cmd_erase(dev, &chunk_addr, 1, descr, erase_mode, &ret);
	if (res < 0) {
		CU_FAIL("Erase failure");
		return;
	}

	if (use_metadata) {
		struct nvm_spec_rprt_descr orig = rprt->descr[chunk_addr.l.chunk];

		CU_ASSERT(descr->cs == orig.cs);
		CU_ASSERT(descr->ct == orig.ct);
		CU_ASSERT(descr->wli >= orig.wli);
		CU_ASSERT(descr->addr == orig.addr);
		CU_ASSERT(descr->naddrs == orig.naddrs);
		CU_ASSERT(descr->wp == 0);

		nvm_buf_free(descr);
		nvm_buf_free(rprt);
	}
}

void test_ERASE_VECTOR_S20_META1(void)
{
	switch(nvm_dev_get_verid(dev)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		erase_s20(1, NVM_CMD_VECTOR);
		break;

	default:
		CU_FAIL("invalid verid");
	}
}


void test_ERASE_VECTOR_S20_META0(void)
{
	switch(nvm_dev_get_verid(dev)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		erase_s20(0, NVM_CMD_VECTOR);
		break;

	default:
		CU_FAIL("invalid verid");
	}
}

void test_ERASE_SCALAR_S20_NADDRS1(void)
{
	switch(nvm_dev_get_verid(dev)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		erase_s20(0, NVM_CMD_SCALAR);
		break;

	default:
		CU_FAIL("invalid verid");
	}
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_cmd_erase_vector",
					argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "ERASE_VECTOR_S20_META1", test_ERASE_VECTOR_S20_META1))
		goto out;
	if (!CU_add_test(pSuite, "ERASE_VECTOR_S20_META0", test_ERASE_VECTOR_S20_META0))
		goto out;

	if (!CU_add_test(pSuite, "ERASE_SCALAR_S20_NADDRS1", test_ERASE_SCALAR_S20_NADDRS1))
		goto out;
	/*
	// Some arbitrary size in the range [1,MAX]
	if (!CU_add_test(pSuite, "ERASE_SCALAR_S20_NADDRS_ARB", test_ERASE_SCALAR_S20_NADDRS_ARB))
		goto out;

	// Max allowed according to NVMe SPEC
	if (!CU_add_test(pSuite, "ERASE_SCALAR_S20_NADDRS_MAX", test_ERASE_SCALAR_S20_NADDRS_MAX))
		goto out;

	// Larger than MAX allowed according to NVMe SPEC, negative test, should
	// fail with EINVAL
	if (!CU_add_test(pSuite, "ERASE_SCALAR_S20_NADDRS_BAD", test_ERASE_SCALAR_S20_NADDRS_BAD))
		goto out;
	*/

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
