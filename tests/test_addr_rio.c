#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

#define SEED 1337

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int ch_bgn = 0;
static int ch_end = 0;
static int lun_bgn = 0;
static int lun_end = 0;
static int block = 10;

static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_vblk *blk;

static int VERBOSE = 0;

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

void print_mismatch(char *expected, char *actual, size_t nbytes)
{
	printf("MISMATCHES:\n");
	for (int i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			printf("i(%06d), expected(%c) != actual(%02d|0x%02x|%c)\n",
				i, expected[i], (int)actual[i], (int)actual[i], actual[i]);
		}
	}
}

struct nvm_addr vblk_off2addr(struct nvm_vblk *vblk, size_t offset)
{
	const struct nvm_addr *blks = nvm_vblk_get_addrs(vblk);
	int nblks = nvm_vblk_get_naddrs(vblk);

	const int SPAGE_NADDRS = geo->nplanes * geo->nsectors;
	const int ALIGN = SPAGE_NADDRS * geo->sector_nbytes;
	const int SPAGE = offset / ALIGN;

	const int idx = SPAGE % nblks;
	
	struct nvm_addr addr = blks[idx];

	addr.g.pg = (SPAGE / nblks) % geo->npages;
	addr.g.pl = ((offset / geo->sector_nbytes) / geo->nsectors) % geo->nplanes;
	addr.g.sec = (offset / geo->sector_nbytes) % geo->nsectors;

	return addr;
}

int setup(void)
{
	size_t buf_nbytes;
	ssize_t res;
	char *buf;

	srand(SEED);

	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
	}
	geo = nvm_dev_get_geo(dev);

	blk = nvm_vblk_alloc_line(dev, ch_bgn, ch_end, lun_bgn, lun_end, block);
	if (!blk) {
		errno = ENOMEM;
		return -1;
	}

	if (VERBOSE)
		nvm_vblk_pr(blk);

	buf_nbytes = nvm_vblk_get_nbytes(blk);

	buf = nvm_buf_alloc(geo, buf_nbytes);
	if (!buf) {
		nvm_vblk_free(blk);

		errno = ENOMEM;
		return -1;
	}

	// Fill buf
	for (size_t ofz = 0; ofz < buf_nbytes; ofz += geo->sector_nbytes) {
		char sec_buf[geo->sector_nbytes];
		size_t sec_len;

		sprintf(sec_buf, "--={(0x%016lx)}=--",
			vblk_off2addr(blk, ofz).ppa);
		sec_len = strlen(sec_buf);

		nvm_buf_fill(buf + ofz, geo->sector_nbytes);
		memcpy(buf + ofz, sec_buf, sec_len);
	}

	res = nvm_vblk_erase(blk);
	if (res < 0) {
		nvm_vblk_free(blk);
		free(buf);

		errno = EIO;
		return -1;
	}

	res = nvm_vblk_write(blk, buf, buf_nbytes);
	if (res < 0) {
		nvm_vblk_free(blk);
		free(buf);

		errno = EIO;
		return -1;
	}

	free(buf);
	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);
	nvm_vblk_free(blk);

	return 0;
}

void _test_NADDR(int naddrs, int pmode)
{
	struct nvm_addr addrs[naddrs];
	size_t blk_nbytes = nvm_vblk_get_nbytes(blk);
	size_t cmd_nbytes = naddrs * geo->sector_nbytes;
	size_t ncmds = blk_nbytes / cmd_nbytes;
	char* cmd_buf;
	ssize_t res;

	cmd_buf = nvm_buf_alloc(geo, cmd_nbytes);
	if (!cmd_buf) {
		CU_FAIL("FAILED: nvm_buf_alloc");
		return;
	}

	size_t nchannels = (ch_end - ch_bgn) + 1;
	size_t nluns = (lun_end - lun_bgn) + 1;
	size_t nplanes = geo->nplanes;
	size_t npages = geo->npages;
	size_t nsectors = geo->nsectors;

	for (int cmd = 0; cmd < ncmds; ++cmd) {
		struct nvm_ret ret = {};

		for (int i = 0; i < naddrs; ++i) {
			int a_rand = rand();

			addrs[i].ppa = 0;
			addrs[i].g.ch = a_rand % nchannels;
			addrs[i].g.lun = a_rand % nluns;
			addrs[i].g.pg = a_rand % npages;
			addrs[i].g.pl = a_rand % nplanes;
			addrs[i].g.sec = a_rand % nsectors;

			addrs[i].g.blk = block;
		}

		memset(cmd_buf, 0, cmd_nbytes);
		res = nvm_addr_read(dev, addrs, naddrs, cmd_buf, NULL, pmode,
				    &ret);
		CU_ASSERT(res==0)
		if (res) {
			if (VERBOSE) {
				nvm_addr_prn(addrs, naddrs);
				nvm_ret_pr(&ret);
			}
			CU_FAIL("FAILED: nvm_addr_read");
			break;
		}

		char sec_buf[geo->sector_nbytes];	// Fill sec_buf
		nvm_buf_fill(sec_buf, geo->sector_nbytes);

		for (int i = 0; i < naddrs; ++i) {	// Fill sec# and compare
			struct nvm_addr addr = addrs[i];
			char tmp[geo->sector_nbytes];

			sprintf(tmp, "--={(0x%016lx)}=--", addr.ppa);
			memcpy(sec_buf, tmp, strlen(tmp));

			if (compare_buffers(sec_buf,
					    cmd_buf + i * geo->sector_nbytes,
					    geo->sector_nbytes)) {
				CU_FAIL("FAILED: Read verification");
				if (VERBOSE) {
					nvm_addr_pr(addr);
					print_mismatch(sec_buf,
						       cmd_buf + i * geo->sector_nbytes,
						       geo->sector_nbytes);
				}
			}
		}
	}
}

void test_NADDR_1_SNGL(void)
{
	_test_NADDR(1, NVM_FLAG_PMODE_SNGL);
}

void test_NADDR_2_SNGL(void)
{
	_test_NADDR(2, NVM_FLAG_PMODE_SNGL);
}

void test_NADDR_MAX_SNGL(void)
{
	_test_NADDR(NVM_NADDR_MAX, NVM_FLAG_PMODE_SNGL);
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 8:
		VERBOSE = atoi(argv[7]);
	case 7:
		block = atoi(argv[6]);
	case 6:
		lun_end = atoi(argv[5]);
	case 5:
		lun_bgn = atoi(argv[4]);
	case 4:
		ch_end = atoi(argv[3]);
	case 3:
		ch_bgn = atoi(argv[2]);
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
	(NULL == CU_add_test(pSuite, "NADDR(1) SNGL", test_NADDR_1_SNGL)) ||
	(NULL == CU_add_test(pSuite, "NADDR(2) SNGL", test_NADDR_2_SNGL)) ||
	(NULL == CU_add_test(pSuite, "NADDR(MAX) SNGL", test_NADDR_MAX_SNGL)) ||
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
