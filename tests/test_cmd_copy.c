/**
 * Minimal test of nvm_cmd_copy
 *
 * Requires / Depends on:
 *
 *  - That the device has two free chunks
 *  - nvm_cmd_rprt_arbs
 *  - nvm_cmd_write
 *  - nvm_cmd_read
 *  - nvm_buf
 *
 * Verifies:
 *
 *  - nvm_cmd_copy can submit and complete without error
 *
 * Two chunks, a source and a destination, are selected by consulting
 * nvm_cmd_rprt_get_arbs one is written with a constructed payload, then copied
 * to the other chunk, read and buffers compared
 */
#include "test_intf.c"

#define SRC 0
#define DST 1
#define NCHUNKS 2

int cmd_copy(int cmd_opt)
{
	const size_t io_nsectr = nvm_dev_get_ws_opt(DEV);
	size_t bufs_nbytes = GEO->l.nsectr * GEO->l.nbytes;
	struct nvm_buf_set *bufs = NULL;
	struct nvm_addr chunks[NCHUNKS];
	int res = -1;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, NCHUNKS, chunks)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		goto exit;
	}

	bufs = nvm_buf_set_alloc(DEV, bufs_nbytes, 0);
	if (!bufs) {
		CU_FAIL("nvm_buf_set_alloc");
		goto exit;
	}
	nvm_buf_set_fill(bufs);

	for (size_t sectr = 0; sectr < GEO->l.nsectr; sectr += io_nsectr) {
		const size_t buf_ofz = sectr * GEO->l.nbytes;
		struct nvm_addr src[io_nsectr];

		for (size_t idx = 0; idx < io_nsectr; ++idx) {
			src[idx] = chunks[SRC];
			src[idx].l.sectr = sectr + idx;
		}

		res = nvm_cmd_write(DEV, src, io_nsectr,
				    bufs->write + buf_ofz, NULL,
				    cmd_opt, NULL);
		if (res < 0) {
			CU_FAIL("nvm_cmd_write");
			goto exit;
		}
	}

	for (size_t sectr = 0; sectr < GEO->l.nsectr; sectr += io_nsectr) {
		struct nvm_addr src[io_nsectr];
		struct nvm_addr dst[io_nsectr];

		for (size_t idx = 0; idx < io_nsectr; ++idx) {
			src[idx] = chunks[SRC];
			src[idx].l.sectr = sectr + idx;

			dst[idx] = chunks[DST];
			dst[idx].l.sectr = sectr + idx;
		}

		res = nvm_cmd_copy(DEV, src, dst, io_nsectr, 0, NULL);
		if (res < 0) {
			CU_FAIL("nvm_cmd_copy");
			goto exit;
		}
	}

	for (size_t sectr = 0; sectr < GEO->l.nsectr; sectr += io_nsectr) {
		const size_t buf_ofz = sectr * GEO->l.nbytes;
		struct nvm_addr dst[io_nsectr];

		for (size_t idx = 0; idx < io_nsectr; ++idx) {
			dst[idx] = chunks[DST];
			dst[idx].l.sectr = sectr + idx;
		}

		res = nvm_cmd_read(DEV, dst, io_nsectr,
				   bufs->read + buf_ofz, NULL,
				   cmd_opt, NULL);
		if (res < 0) {
			CU_FAIL("nvm_cmd_read");
			goto exit;
		}
	}

	if (nvm_buf_diff(bufs->read, bufs->write, bufs->nbytes)) {
		CU_FAIL("buffer mismatch");
		goto exit;
	}

	CU_PASS("Success");
	res = 0;

exit:
	nvm_buf_set_free(bufs);
	return res;
}

static void test_CMD_COPY_SWR(void)
{
	CU_ASSERT(!cmd_copy(NVM_CMD_SCALAR));
}

static void test_CMD_COPY_VWR(void)
{
	CU_ASSERT(!cmd_copy(NVM_CMD_VECTOR));
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_cmd_copy", argc, argv, 0);
	if (!pSuite)
		goto out;

	if (!CU_ADD_TEST(pSuite, test_CMD_COPY_VWR))
		goto out;

	if (!CU_ADD_TEST(pSuite, test_CMD_COPY_SWR))
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
