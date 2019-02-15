#include "test_intf.c"

#define KB (0x1ull << 10)
#define MB (KB * 1024)
#define GB (MB * 1024)

#define PSEUDO_ALIGN KB * 4

static size_t bsizes[] = {
	KB, 4 * KB, 256 * KB,
	MB, 128 * MB, 512 * MB,
	GB, 2 * GB,
};
static size_t nbsizes = sizeof(bsizes) / sizeof(bsizes[0]);

static void test_BUF_VIRT(void) {
	for (size_t i = 0; i < nbsizes; ++i) {
		void *buf = NULL;

		buf = nvm_buf_virt_alloc(PSEUDO_ALIGN, bsizes[i]);
		CU_ASSERT_PTR_NOT_NULL(buf);
		nvm_buf_virt_free(buf);
	}
}

static void test_BUF(void) {
	for (size_t i = 0; i < nbsizes; ++i) {
		void *buf = NULL;

		buf = nvm_buf_alloc(DEV, bsizes[i], NULL);
		CU_ASSERT_PTR_NOT_NULL(buf);
		nvm_buf_free(DEV, buf);
	}
}

static void test_BUF_SET(void) {

	for (size_t i = 0; i < nbsizes; ++i) {
		struct nvm_buf_set *bufs = NULL;

		bufs = nvm_buf_set_alloc(DEV, bsizes[i], 0);
		CU_ASSERT_PTR_NOT_NULL(bufs);
		nvm_buf_set_free(bufs);
	}
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_cmd_erase_vector", argc, argv, 0);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "BUF_VIRT", test_BUF_VIRT))
		goto out;

	if (!CU_add_test(pSuite, "BUF", test_BUF))
		goto out;

	if (!CU_add_test(pSuite, "BUF_SET", test_BUF_SET))
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
