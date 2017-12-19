#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>
#include <CUnit/Basic.h>

static int rmode = CU_BRM_VERBOSE;
static int seed = 0;
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;

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
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
	}
	geo = nvm_dev_get_geo(dev);

	srand(seed);
}

int teardown(void)
{
	nvm_dev_close(dev);

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

	for (size_t cmd = 0; cmd < ncmds; ++cmd) {
		struct nvm_ret ret = {0,0};

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
				nvm_addr_prn(addrs, naddrs, dev);
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

			if (nvm_buf_diff(sec_buf,
					    cmd_buf + i * geo->sector_nbytes,
					    geo->sector_nbytes)) {
				CU_FAIL("FAILED: Read verification");
				if (VERBOSE) {
					nvm_addr_pr(addr);
					nvm_buf_diff_pr(sec_buf,
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
	seed = time(NULL);		// Default arbitrary seed
	switch(argc) {
	case 4:
		switch(atoi(argv[3])) {
		case 2:
			rmode = CU_BRM_VERBOSE;
			break;
		case 1:
			rmode = CU_BRM_SILENT;
			break;
		case 0:
		default:
			rmode = CU_BRM_NORMAL;
			break;
		}
	case 3:
		seed = atoi(argv[2]);	// Overwrite default arbitrary seed
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return 1;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}

	printf("# TEST_INPUT: {dev: '%s', seed: %u, rmode: 0x%x}\n",
	       nvm_dev_path, seed, rmode);

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
