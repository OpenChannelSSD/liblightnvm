#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include <liblightnvm.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
	}
	geo = nvm_dev_get_geo(dev);

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);

	return 0;
}

static size_t compare_buffers(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	for (size_t i = 0; i < nbytes; ++i)
		if (expected[i] != actual[i])
			++diff;

	return diff;
}

static void print_mismatch(char *expected, char *actual, size_t nbytes)
{
	printf("MISMATCHES:\n");
	for (size_t i = 0; i < nbytes; ++i)
		if (expected[i] != actual[i])
			printf("i(%06lu), exp(%c) != act(%02d|0x%02x|%c)\n",
				i, expected[i], (int)actual[i],
				(int)actual[i], actual[i]);
}

/**
 * Find an arbitrary chunk in the given state on the given device
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
static int nvm_cmd_rprt_arbc(struct nvm_dev *dev, int cs, struct nvm_addr *addr)
{
	struct nvm_spec_rprt *rprt = nvm_cmd_rprt(dev, NULL, 0x0, 0x0);

	if (!rprt)
		return -1;

	printf("### Looking for chunk...\n");
	for (size_t idx = rprt->ndescr / 2; idx < rprt->ndescr; ++idx) {
		if (rprt->descr[idx].chunk_state != cs)
			continue;
	
		printf("idx: %zu\n", idx);
		*addr = nvm_addr_dev2gen(dev, rprt->descr[idx].chunk_addr);
		free(rprt);
		return 0;
	}

	free(rprt);
	return -1;
}

static void erase_write_read(int use_meta)
{
	const int naddrs = nvm_dev_get_ws_min(dev);
	struct nvm_addr addrs[naddrs];
	struct nvm_buf_set *bufs = { 0 };
	struct nvm_ret ret;
	struct nvm_addr chunk_addr = { .val = 0 };
	int failed = 1;
	ssize_t res;

	printf("INFO: N naddrs(%d), use_meta(%d) on ", naddrs, use_meta);

	if (nvm_cmd_rprt_arbc(dev, NVM_RPRT_FREE, &chunk_addr)) {
		CU_FAIL("nvm_cmd_rprt_arbc");
		goto out;
	}

	nvm_addr_print(&chunk_addr, 1, dev);

	bufs = nvm_buf_set_alloc(dev, naddrs * geo->sector_nbytes,
				 use_meta ? naddrs * geo->l.nbytes_oob : 0);
	if (!bufs) {
		CU_FAIL("nvm_buf_set_alloc");
		goto out;
	}
	nvm_buf_set_fill(bufs);

	///< Erase
	res = nvm_cmd_erase(dev, addrs, 1, 0x0, &ret);
	if (res < 0) {
		CU_FAIL("Erase failure");
		goto out;
	}

	///< Write
	for (size_t sectr = 0; sectr < geo->l.nsectr; sectr += naddrs) {
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].val = chunk_addr.val;
			addrs[i].l.sectr = sectr + i;
		}
		res = nvm_cmd_write(dev, addrs, naddrs, bufs->write,
				    bufs->write_meta, 0x0, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto out;
		}
	}

	///< Read
	for (size_t sectr = 0; sectr < geo->l.nsectr; sectr += naddrs) {
		size_t buf_diff = 0;
		size_t meta_diff = 0;

		for (int i = 0; i < naddrs; ++i) {
			addrs[i].val = chunk_addr.val;
			addrs[i].l.sectr = sectr + i;
		}

		// Zero out read buffers
		memset(bufs->read, 0, bufs->nbytes);
		if (use_meta)
			memset(bufs->read_meta, 0, bufs->nbytes_meta);

		res = nvm_cmd_read(dev, addrs, naddrs, bufs->read,
				   bufs->read_meta, 0x0, &ret);
		if (res < 0) {
			CU_FAIL("Read failure: command error");
			goto out;
		}

		buf_diff = compare_buffers(bufs->read, bufs->write, bufs->nbytes);
		if (buf_diff) {
			CU_FAIL("Read failure: buffer mismatch");
			printf("buf_diff: %zu\n", buf_diff);
		}

		if (use_meta) {
			meta_diff = compare_buffers(bufs->read_meta, bufs->write_meta, bufs->nbytes_meta);
			if (meta_diff) {
				CU_FAIL("Read failure: meta mismatch");
				print_mismatch(bufs->write_meta, bufs->read_meta, bufs->nbytes_meta);
			}
		}

		if (buf_diff || meta_diff) {
			CU_FAIL("buf_diff || meta_diff");
			goto out;
		}
	}

	CU_PASS("Success");
	failed = 0;

out:
	nvm_buf_set_free(bufs);

	if (failed)
		printf("Failure on ADDR(0x%016lx)\n", chunk_addr.val);
}

void test_CMD_EWR_META0(void)
{
	erase_write_read(0);
}

void test_CMD_EWR_META1(void)
{
	erase_write_read(1);
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return 1;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}
	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_cmd_{erase,write,read}*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "EWR - META", test_CMD_EWR_META0)) ||
	(NULL == CU_add_test(pSuite, "EWR + META", test_CMD_EWR_META1)) ||
	0)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_NORMAL);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
