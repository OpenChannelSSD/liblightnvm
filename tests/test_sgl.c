#include "test_util.h"
#include "test_intf.c"

static void _test_sgl_rw(struct nvm_addr slba, char *buf, char *meta,
	size_t nsectr, uint16_t flags, uint8_t write)
{
	size_t nbytes = nsectr * SECTOR_SIZE;
	size_t nbytes_meta = nsectr * OOB_SIZE;

	char *bufp, *metap;
	int rc;

	struct nvm_addr addrs[WS_OPT];

	struct nvm_sgl_pool *pool = nvm_sgl_pool_create(DEV);

	for (size_t sectr = 0; sectr < nsectr; sectr += WS_OPT) {
		struct nvm_sgl *sgl = nvm_sgl_alloc(pool);
		struct nvm_sgl *sgl_meta = meta ? nvm_sgl_alloc(pool) : NULL;

		for (uint32_t i = 0; i < WS_OPT; i++) {
			bufp = (i % 2 == 0) ? buf : buf + nbytes / 2;
			nvm_sgl_add(DEV, sgl, bufp + (sectr + i) / 2 * SECTOR_SIZE,
				SECTOR_SIZE);

			if (meta) {
				metap = (i % 2 == 0) ? meta :
					meta + nbytes_meta / 2;
				nvm_sgl_add(DEV, sgl_meta,
					metap + (sectr + i) / 2 * OOB_SIZE,
					OOB_SIZE);
			}
		}

		slba.l.sectr = sectr;

		if (flags & NVM_CMD_VECTOR) {
			nvm_addr_fill_crange(addrs, slba, WS_OPT);
		} else {
			addrs[0] = slba;
		}

		if (write) {
			rc = nvm_cmd_write(DEV, addrs, WS_OPT, sgl, sgl_meta,
				flags, NULL);
		} else {
			rc = nvm_cmd_read(DEV, addrs, WS_OPT, sgl, sgl_meta,
				flags, NULL);
		}

		CU_ASSERT_FATAL(rc == 0);
		nvm_sgl_free(pool, sgl);
		if (meta) nvm_sgl_free(pool, sgl_meta);
	}

	nvm_sgl_pool_destroy(pool);
}

static void _test_sgl_read(struct nvm_addr addr, char *buf, char *meta,
	size_t nsectr, uint16_t flags)
{
	_test_sgl_rw(addr, buf, meta, nsectr, flags, 0);
}

static void _test_sgl_write(struct nvm_addr addr, char *buf, char *meta,
	size_t nsectr, uint16_t flags)
{
	_test_sgl_rw(addr, buf, meta, nsectr, flags, 1);
}

static void _test_sgl(size_t nsectr_w, size_t nsectr_r, uint16_t flags)
{
	struct nvm_addr addr;

	size_t nbytes_w = nsectr_w * SECTOR_SIZE;
	size_t nbytes_r = nsectr_r * SECTOR_SIZE;
	size_t nbytes_meta_w = nsectr_w * OOB_SIZE;
	size_t nbytes_meta_r = nsectr_r * OOB_SIZE;

	char *buf_w, *buf_r, *meta_w = NULL, *meta_r = NULL;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
	}

	buf_w = nvm_buf_alloc(DEV, nbytes_w, NULL);
	buf_r = nvm_buf_alloc(DEV, nbytes_r, NULL);

	memset(buf_w, 'A', nbytes_w / 2);
	memset(buf_w+nbytes_w / 2, 'B', nbytes_w / 2);

	if (flags & NVM_CMD_SGL_META) {
		meta_w = nvm_buf_alloc(DEV, nbytes_meta_w, NULL);
		meta_r = nvm_buf_alloc(DEV, nbytes_meta_r, NULL);

		memset(meta_w, 'C', nbytes_meta_w / 2);
		memset(meta_w+nbytes_meta_w / 2, 'D', nbytes_meta_w / 2);
	}

	_test_sgl_write(addr, buf_w, meta_w, nsectr_w, flags);
	_test_sgl_read(addr, buf_r, meta_r, nsectr_r, flags);

	CU_ASSERT(memcmp(buf_r, buf_w, nbytes_r / 2) == 0);
	CU_ASSERT(memcmp(buf_r + (nbytes_r / 2), buf_w + (nbytes_w / 2),
		nbytes_r / 2) == 0);

	nvm_buf_free(DEV, buf_w);
	nvm_buf_free(DEV, buf_r);

	if (flags & NVM_CMD_SGL_META) {
		CU_ASSERT(memcmp(meta_r, meta_w, nbytes_meta_r / 2) == 0);
		CU_ASSERT(memcmp(meta_r + (nbytes_meta_r / 2),
			meta_w + (nbytes_meta_w / 2), nbytes_meta_r / 2) == 0);

		nvm_buf_free(DEV, meta_w);
		nvm_buf_free(DEV, meta_r);
	}
}


#define MAKE_TEST(type, name, nsectr_w, nsectr_r, flags)                      \
	static void test_sgl_ ## type ## _ ## name (void)                     \
	{                                                                     \
		_test_sgl(nsectr_w, nsectr_r, flags);                         \
	}

#define MAKE_TESTS(name, nsectr_w, nsectr_r) \
	MAKE_TEST(scalar, name ## _meta, nsectr_w, nsectr_r, NVM_CMD_SCALAR | NVM_CMD_SGL | NVM_CMD_SGL_META) \
	MAKE_TEST(vector, name ## _meta, nsectr_w, nsectr_r, NVM_CMD_VECTOR | NVM_CMD_SGL | NVM_CMD_SGL_META) \


MAKE_TESTS(ws_opt, WS_OPT + MW_CUNITS, WS_OPT)
MAKE_TESTS(nsectr, NSECTR, NSECTR)

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("sgl",
					argc, argv);
	if (!pSuite)
		goto out;

	switch (BE_ID) {
	case NVM_BE_NOCD:
	case NVM_BE_SPDK:
		if (!CU_add_test(pSuite, "simple: {mode: SCALAR; nsectr: WS_OPT; metadata: ON}", test_sgl_scalar_ws_opt_meta))
			goto out;
		if (!CU_add_test(pSuite, "simple: {mode: SCALAR; nsectr: NSECTR; metadata: ON}", test_sgl_scalar_nsectr_meta))
			goto out;

		if (!CU_add_test(pSuite, "simple: {mode: VECTOR; nsectr: WS_OPT; metadata: ON}", test_sgl_vector_ws_opt_meta))
			goto out;
		if (!CU_add_test(pSuite, "simple: {mode: VECTOR; nsectr: NSECTR; metadata: ON}", test_sgl_vector_nsectr_meta))
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
