#include "test_intf.c"

/**
 * @param use_meta Set to 1 to allocate, write, read and verify meta-data buffer
 * @param erase_mode set to NVM_CMD_SCALAR or to NVM_CMD_VECTOR
 */
static void ewr(int use_meta, int erase_cmd_mode)
{
	const int naddrs = nvm_dev_get_ws_min(DEV);
	struct nvm_buf_set *bufs = NULL;
	struct nvm_ret ret;
	struct nvm_addr chunk_addr = { .val = 0 };
	ssize_t res;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &chunk_addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		goto failure;
	}

	bufs = nvm_buf_set_alloc(DEV, naddrs * GEO->l.nbytes,
				 use_meta ? naddrs * GEO->l.nbytes_oob : 0);
	if (!bufs) {
		CU_FAIL("nvm_buf_set_alloc");
		goto failure;
	}
	nvm_buf_set_fill(bufs);

	// Write the chunk
	for (size_t sectr = 0; sectr < GEO->l.nsectr; sectr += naddrs) {
		struct nvm_addr addr = chunk_addr;

		addr.l.sectr = sectr;

		res = nvm_cmd_write(DEV, &addr, naddrs, bufs->write,
				    bufs->write_meta, NVM_CMD_SCALAR, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto failure;
		}
	}

	// Read
	for (size_t sectr = 0; sectr < GEO->l.nsectr; sectr += naddrs) {
		struct nvm_addr addr = chunk_addr;

		size_t buf_diff = 0;
		size_t meta_diff = 0;

		addr.l.sectr = sectr;

		memset(bufs->read, 0, bufs->nbytes);	///< Reset read buffers
		if (use_meta)
			memset(bufs->read_meta, 0, bufs->nbytes_meta);

		res = nvm_cmd_read(DEV, &addr, naddrs, bufs->read,
				   bufs->read_meta, NVM_CMD_SCALAR, &ret);
		if (res < 0) {
			CU_FAIL("Read failure: command error");
			goto failure;
		}

		buf_diff = nvm_buf_diff(bufs->read, bufs->write, bufs->nbytes);
		if (buf_diff) {
			CU_FAIL("Read failure: buffer mismatch");
		}

		if (use_meta) {
			meta_diff = nvm_buf_diff(bufs->read_meta,
						 bufs->write_meta,
						 bufs->nbytes_meta);
			if (meta_diff) {
				CU_FAIL("Read failure: meta mismatch");
				if (CU_BRM_VERBOSE == RMODE) {
					nvm_buf_diff_pr(bufs->write_meta,
							bufs->read_meta,
							bufs->nbytes_meta);
				}
			}
		}

		if (buf_diff || meta_diff) {
			CU_FAIL("buf_diff || meta_diff");
			goto failure;
		}
	}

	// Erase the chunk
	res = nvm_cmd_erase(DEV, &chunk_addr, 1, NULL, erase_cmd_mode, &ret);
	if (res < 0) {
		CU_FAIL("Erase failure");
		goto failure;
	}

	CU_PASS("Success");

failure:
	nvm_buf_set_free(bufs);
}

void test_EWR_SSS(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("Nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr(0, NVM_CMD_SCALAR);
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_SSS_META1(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("Nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr(1, NVM_CMD_SCALAR);
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_VSS(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("Nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr(0, NVM_CMD_VECTOR);
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_VSS_META1(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("Nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr(1, NVM_CMD_VECTOR);
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_test_cmd_wre_scalar", argc, argv,
					0);
	if (!pSuite)
		goto out;

	switch (BE_ID) {
		case NVM_BE_IOCTL:
		case NVM_BE_SPDK:
			if (!CU_add_test(pSuite, "EWR_SSS_META", test_EWR_SSS_META1))
				goto out;
			if (!CU_add_test(pSuite, "EWR_VSS_META", test_EWR_VSS_META1))
				goto out;
			/* fallthrough */
		case NVM_BE_LBD:
			if (!CU_add_test(pSuite, "EWR_SSS", test_EWR_SSS))
				goto out;
			if (!CU_add_test(pSuite, "EWR_VSS", test_EWR_VSS))
				goto out;
	}

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
