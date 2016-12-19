#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int channel = 0;
static int lun = 0;
static int block = 10;

static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr blk_addr;

size_t compare_buffers(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	for (int i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			++diff;
		}
	}

	return diff;
}

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
	}
	geo = nvm_dev_attr_geo(dev);

	blk_addr.ppa = 0;
	blk_addr.g.ch = channel;
	blk_addr.g.lun = lun;
	blk_addr.g.blk = block;

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);

	return 0;
}

void _test_1ADDR(int use_meta)
{
	char *buf_w = NULL, *buf_r = NULL, *meta_w = NULL, *meta_r = NULL;
	const int naddrs = geo->nplanes * geo->nsectors;
	struct nvm_addr addrs[naddrs];
	struct nvm_ret ret;
	ssize_t res;
	size_t buf_w_nbytes, meta_w_nbytes, buf_r_nbytes, meta_r_nbytes;
	int pmode = NVM_FLAG_PMODE_SNGL;
	int failed = 1;

	buf_w_nbytes = naddrs * geo->sector_nbytes;
	meta_w_nbytes = naddrs * geo->meta_nbytes;
	buf_r_nbytes = geo->sector_nbytes;
	meta_r_nbytes = geo->meta_nbytes;

	buf_w = nvm_buf_alloc(geo, buf_w_nbytes);	// Setup buffers
	if (!buf_w) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}
	nvm_buf_fill(buf_w, buf_w_nbytes);

	meta_w = nvm_buf_alloc(geo, meta_w_nbytes);
	if (!meta_w) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}
	for (int i = 0; i < meta_w_nbytes; ++i) {
		meta_w[i] = 65;
	}
	for (int i = 0; i < naddrs; ++i) {
		char meta_descr[meta_w_nbytes];
		int sec = i % geo->nsectors;
		int pl = (i / geo->nsectors) % geo->nplanes;

		sprintf(meta_descr, "[P(%02d),S(%02d)]", pl, sec);
		if (strlen(meta_descr) > geo->meta_nbytes) {
			CU_FAIL("Failed constructing meta buffer");
			goto exit_naddr;
		}

		memcpy(meta_w + i * geo->meta_nbytes, meta_descr,
		       strlen(meta_descr));
	}

	buf_r = nvm_buf_alloc(geo, buf_r_nbytes);
	if (!buf_r) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}

	meta_r = nvm_buf_alloc(geo, buf_r_nbytes);
	if (!meta_r) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}

	for (int pl = 0; pl < geo->nplanes; ++pl) {	// Erase
		addrs[pl].ppa = blk_addr.ppa;

		addrs[pl].g.pl = pl;
	}

	res = nvm_addr_erase(dev, addrs, geo->nplanes, pmode, &ret);
	if (res < 0) {
		CU_FAIL("Erase failure");
		goto exit_naddr;
	}

	for (int pg = 0; pg < geo->npages; ++pg) {	// Write
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.sec = i % geo->nsectors;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
		}
		res = nvm_addr_write(dev, addrs, naddrs, buf_w,
				     use_meta ? meta_w : NULL, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto exit_naddr;
		}
	}

	for (int pg = 0; pg < geo->npages; ++pg) {		// Read
		for (int pl = 0; pl < geo->nplanes; ++pl) {
			for (int sec = 0; sec < geo->nsectors; ++sec) {
				struct nvm_addr addr;
				size_t buf_diff = 0, meta_diff = 0;

				int bw_offset = sec * geo->sector_nbytes + \
					       pl * geo->nsectors * geo->sector_nbytes;
				int mw_offset = sec * geo->meta_nbytes + \
						pl * geo->nsectors * geo->meta_nbytes;

				addr.ppa = blk_addr.ppa;
				addr.g.pg = pg;
				addr.g.pl = pl;
				addr.g.sec = sec;

				memset(buf_r, 0, buf_r_nbytes);
				memset(meta_r, 0, meta_r_nbytes);

				res = nvm_addr_read(dev, &addr, 1, buf_r,
						    use_meta ? meta_r : NULL, pmode, &ret);
				if (res < 0) {
					CU_FAIL("Read failure");
					goto exit_naddr;
				}

				buf_diff = compare_buffers(buf_r, buf_w + bw_offset, buf_r_nbytes);
				if (use_meta)
					meta_diff = compare_buffers(meta_r, meta_w + mw_offset, meta_r_nbytes);
				
				if (buf_diff)
					CU_FAIL("Read failure: buffer mismatch");
				if (meta_diff)
					CU_FAIL("Read failure: meta mismatch");
				if (buf_diff || meta_diff)
					goto exit_naddr;
			}
		}
	}

	CU_PASS("Success");
	failed = 0;

exit_naddr:
	free(meta_r);
	free(buf_r);
	free(meta_w);
	free(buf_w);

	if (failed)
		printf("Failure on PPA(0x%016lx)\n", blk_addr.ppa);

	return;
}

void test_1ADDR_META0_SNGL(void)
{
	_test_1ADDR(0);
}

void test_1ADDR_META1_SNGL(void)
{
	_test_1ADDR(1);
}

void _test_NADDR(int use_meta, int pmode)
{
	char *buf_w = NULL, *buf_r = NULL, *meta_w = NULL, *meta_r = NULL;
	const int naddrs = geo->nplanes * geo->nsectors;
	struct nvm_addr addrs[naddrs];
	struct nvm_ret ret;
	ssize_t res;
	size_t buf_nbytes, meta_nbytes;
	int failed = 1;

	buf_nbytes = naddrs * geo->sector_nbytes;
	meta_nbytes = naddrs * geo->meta_nbytes;

	buf_w = nvm_buf_alloc(geo, buf_nbytes);	// Setup buffers
	if (!buf_w) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}
	nvm_buf_fill(buf_w, buf_nbytes);

	meta_w = nvm_buf_alloc(geo, meta_nbytes);
	if (!meta_w) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}
	for (int i = 0; i < meta_nbytes; ++i) {
		meta_w[i] = 65;
	}
	for (int i = 0; i < naddrs; ++i) {
		char meta_descr[meta_nbytes];
		int sec = i % geo->nsectors;
		int pl = (i / geo->nsectors) % geo->nplanes;

		sprintf(meta_descr, "[P(%02d),S(%02d)]", pl, sec);
		if (strlen(meta_descr) > geo->meta_nbytes) {
			CU_FAIL("Failed constructing meta buffer");
			goto exit_naddr;
		}

		memcpy(meta_w + i * geo->meta_nbytes, meta_descr,
		       strlen(meta_descr));
	}

	buf_r = nvm_buf_alloc(geo, buf_nbytes);
	if (!buf_r) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}

	meta_r = nvm_buf_alloc(geo, meta_nbytes);
	if (!meta_r) {
		CU_FAIL("nvm_buf_alloc");
		goto exit_naddr;
	}

	for (int pl = 0; pl < geo->nplanes; ++pl) {		// Erase
		addrs[pl].ppa = blk_addr.ppa;

		addrs[pl].g.pl = pl;
	}
	res = nvm_addr_erase(dev, addrs, geo->nplanes, pmode, &ret);
	if (res < 0) {
		CU_FAIL("Erase failure");
		goto exit_naddr;
	}

	for (int pg = 0; pg < geo->npages; ++pg) {		// Write
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.sec = i % geo->nsectors;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
		}
		res = nvm_addr_write(dev, addrs, naddrs, buf_w,
				     use_meta ? meta_w : NULL, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto exit_naddr;
		}
	}

	for (int pg = 0; pg < geo->npages; ++pg) {		// Read
		size_t buf_diff = 0, meta_diff = 0;

		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
			addrs[i].g.sec = i % geo->nsectors;
		}

		memset(buf_r, 0, buf_nbytes);
		if (use_meta)
			memset(meta_r, 0 , meta_nbytes);

		res = nvm_addr_read(dev, addrs, naddrs, buf_r,
				    use_meta ? meta_r : NULL, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Read failure: command error");
			goto exit_naddr;
		}

		buf_diff = compare_buffers(buf_r, buf_w, buf_nbytes);
		if (use_meta)
			meta_diff = compare_buffers(meta_r, meta_w, meta_nbytes);
		
		if (buf_diff)
			CU_FAIL("Read failure: buffer mismatch");
		if (use_meta && meta_diff) {
			CU_FAIL("Read failure: meta mismatch");
			printf("\nExpect(");
			for(int i = 0; i < meta_nbytes; ++i)
				printf("%c", meta_w[i]);
			printf(")\n");
			printf("\nActual(");
			for(int i = 0; i < meta_nbytes; ++i)
				printf("%c", meta_r[i]);
			printf(")\n");
		}
		if (buf_diff || meta_diff)
			goto exit_naddr;
	}

	CU_PASS("Success");
	failed = 0;

exit_naddr:
	free(meta_r);
	free(buf_r);
	free(meta_w);
	free(buf_w);

	if (failed)
		printf("Failure on PPA(0x%016lx)\n", blk_addr.ppa);

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
	(NULL == CU_add_test(pSuite, "NADDR META0 QUAD", test_NADDR_META0_QUAD)) ||
	(NULL == CU_add_test(pSuite, "NADDR META1 QUAD", test_NADDR_META1_QUAD)) ||
	(NULL == CU_add_test(pSuite, "NADDR META0 DUAL", test_NADDR_META0_DUAL)) ||
	(NULL == CU_add_test(pSuite, "NADDR META1 DUAL", test_NADDR_META1_DUAL)) ||
	(NULL == CU_add_test(pSuite, "NADDR META0 SNGL", test_NADDR_META0_SNGL)) ||
	(NULL == CU_add_test(pSuite, "NADDR META1 SNGL", test_NADDR_META1_SNGL)) ||
	(NULL == CU_add_test(pSuite, "1ADDR META0 SNGL", test_1ADDR_META0_SNGL)) ||
	(NULL == CU_add_test(pSuite, "1ADDR META1 SNGL", test_1ADDR_META1_SNGL)) ||
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
