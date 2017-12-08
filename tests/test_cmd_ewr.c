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

struct nvm_buf_set {
	char *write;
	char *write_meta;

	char *read;
	char *read_meta;

	size_t nbytes;
	size_t nbytes_meta;
};

static void nvm_buf_set_free(struct nvm_buf_set *bufs)
{
	if (!bufs)
		return;

	free(bufs->write);
	free(bufs->write_meta);
	free(bufs->read);
	free(bufs->read_meta);
	free(bufs);
}

static void nvm_buf_set_fill(struct nvm_buf_set *bufs)
{
	nvm_buf_fill(bufs->write, bufs->nbytes);
	nvm_buf_fill(bufs->write_meta, bufs->nbytes_meta);

	memset(bufs->read, 0, bufs->nbytes);
	memset(bufs->read_meta, 0, bufs->nbytes_meta);
}

static struct nvm_buf_set *nvm_buf_set_alloc(struct nvm_dev *dev, size_t nbytes,
					     size_t nbytes_meta)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	struct nvm_buf_set *bufs = nvm_buf_alloc(geo, sizeof(*bufs));

	if (!bufs)
		return NULL;

	bufs->nbytes = nbytes;
	bufs->write = malloc(bufs->nbytes);
	bufs->read = malloc(bufs->nbytes);

	bufs->nbytes_meta = nbytes_meta;
	bufs->write_meta = malloc(bufs->nbytes_meta);
	bufs->read_meta = malloc(bufs->nbytes_meta);

	if (!(bufs->write && bufs->write_meta && bufs->read && bufs->read_meta)) {
		nvm_buf_set_free(bufs);
		return NULL;
	}

	return bufs;
}

static erase_write_read(int use_meta)
{
	const int naddrs = nvm_dev_get_ws_min(dev);
	struct nvm_addr addrs[naddrs] = { 0 };
	struct nvm_buf_set *bufs = { 0 };
	struct nvm_ret ret;
	int failed = 1;
	ssize_t res;

	printf("INFO: N naddrs(%d), use_meta(%d) on ", naddrs, use_meta);
	nvm_addr_pr(blk_addr);

	bufs = nvm_buf_set_alloc(dev, naddrs * geo->sector_nbytes,
				 naddrs * geo->nbytes_meta);
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
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.sectr = sectr;
			addrs[i].g.sec = i % geo->nsectors;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
		}
		res = nvm_addr_write(dev, addrs, naddrs, buf_w,
				     use_meta ? meta_w : NULL, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto out;
		}
	}

	///< Read
	for (size_t pg = 0; pg < geo->npages; ++pg) {
		size_t buf_diff = 0, meta_diff = 0;

		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
			addrs[i].g.sec = i % geo->nsectors;
		}

		memset(buf_r, 0, buf_nbytes);
		if (use_meta)
			memset(meta_r, 0 , nbytes_meta);

		res = nvm_addr_read(dev, addrs, naddrs, buf_r,
				    use_meta ? meta_r : NULL, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Read failure: command error");
			goto out;
		}

		buf_diff = compare_buffers(buf_r, buf_w, buf_nbytes);
		if (use_meta)
			meta_diff = compare_buffers(meta_r, meta_w, nbytes_meta);
		
		if (buf_diff)
			CU_FAIL("Read failure: buffer mismatch");
		if (use_meta && meta_diff) {
			CU_FAIL("Read failure: meta mismatch");
			print_mismatch(meta_w, meta_r, nbytes_meta);
		}
		if (buf_diff || meta_diff)
			goto out;
	}

	CU_PASS("Success");
	failed = 0;

out:
	nvm_buf_set_free(bufs);

	if (failed)
		printf("Failure on ADDR(0x%016lx)\n", blk_addr.val);

	return;
}

void test_NADDR_META0_SNGL(void)
{
	++blk_addr.g.blk;
	_test_NADDR(0, NVM_FLAG_PMODE_SNGL);
}

void test_NADDR_META1_SNGL(void)
{
	++blk_addr.g.blk;
	_test_NADDR(1, NVM_FLAG_PMODE_SNGL);
}

void test_NADDR_META0_DUAL(void)
{
	if (geo->nplanes >= 2) {
		++blk_addr.g.blk;
		_test_NADDR(0, NVM_FLAG_PMODE_DUAL);
	}
}

void test_NADDR_META1_DUAL(void)
{
	if (geo->nplanes >= 2) {
		++blk_addr.g.blk;
		_test_NADDR(1, NVM_FLAG_PMODE_DUAL);
	}
}

void test_NADDR_META0_QUAD(void)
{
	if (geo->nplanes >= 4) {
		++blk_addr.g.blk;
		_test_NADDR(0, NVM_FLAG_PMODE_QUAD);
	}
}

void test_NADDR_META1_QUAD(void)
{
	if (geo->nplanes >= 4) {
		++blk_addr.g.blk;
		_test_NADDR(1, NVM_FLAG_PMODE_QUAD);
	}
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 5:
		block = atoi(argv[4]);
	case 4:
		lun = atoi(argv[3]);
	case 3:
		channel = atoi(argv[2]);
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

	pSuite = CU_add_suite("nvm_addr_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "NADDR META1 QUAD", test_NADDR_META1_QUAD)) ||
	(NULL == CU_add_test(pSuite, "NADDR META1 DUAL", test_NADDR_META1_DUAL)) ||
	(NULL == CU_add_test(pSuite, "NADDR META1 SNGL", test_NADDR_META1_SNGL)) ||
	(NULL == CU_add_test(pSuite, "1ADDR META1 SNGL", test_1ADDR_META1_SNGL)) ||
	(NULL == CU_add_test(pSuite, "NADDR META0 QUAD", test_NADDR_META0_QUAD)) ||
	(NULL == CU_add_test(pSuite, "NADDR META0 DUAL", test_NADDR_META0_DUAL)) ||
	(NULL == CU_add_test(pSuite, "NADDR META0 SNGL", test_NADDR_META0_SNGL)) ||
	(NULL == CU_add_test(pSuite, "1ADDR META0 SNGL", test_1ADDR_META0_SNGL)) ||
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
