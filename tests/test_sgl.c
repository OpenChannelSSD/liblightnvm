#include "test_intf.c"

static void test_sgl()
{
	struct nvm_addr addr;
	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
	}

	uint32_t ws_opt = nvm_dev_get_ws_opt(dev);

	size_t sector_size = geo->l.nbytes;
	size_t nbytes = geo->l.nsectr * sector_size;

	struct nvm_buf_set *bufs = nvm_buf_set_alloc(dev, nbytes, 0);
	nvm_buf_set_fill(bufs);

	int rc;

	for (size_t sectr = 0; sectr < geo->l.nsectr; sectr += ws_opt) {
		struct nvm_sgl *sgl = nvm_sgl_alloc();
		for (uint32_t i = 0; i < ws_opt; i++) {
			nvm_sgl_add(sgl, bufs->write + ((sectr+i)*sector_size), sector_size);
		}

		addr.l.sectr = sectr;

		rc = nvm_cmd_write(dev, &addr, ws_opt, sgl, NULL, NVM_CMD_SCALAR | NVM_CMD_SGL, NULL);
		CU_ASSERT(rc == 0);
		nvm_sgl_free(sgl);
	}

	for (size_t sectr = 0; sectr < geo->l.nsectr; sectr += ws_opt) {
		struct nvm_sgl *sgl = nvm_sgl_alloc();
		for (uint32_t i = 0; i < ws_opt; i++) {
			nvm_sgl_add(sgl, bufs->read + ((sectr+i)*sector_size), sector_size);
		}

		addr.l.sectr = sectr;

		rc = nvm_cmd_read(dev, &addr, ws_opt, sgl, NULL, NVM_CMD_SCALAR | NVM_CMD_SGL, NULL);
		CU_ASSERT(rc == 0);
		nvm_sgl_free(sgl);
	}

	int ndiff = nvm_buf_diff(bufs->read, bufs->write, nbytes);
	CU_ASSERT(ndiff == 0);
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("sgl",
					argc, argv);
	if (!pSuite)
		goto out;

	switch (be_id) {
	case NVM_BE_SPDK:
		if (!CU_add_test(pSuite, "simple", test_sgl))
			goto out;
	}

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
