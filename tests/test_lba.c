#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

// Parsed from CLI
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int ch_bgn = 0;
static int ch_end = 0;
static int lun_bgn = 0;
static int lun_end = 0;
static int blk = 0;

// Managed by setup/teardown and used by tests
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
struct nvm_addr bgn;
struct nvm_addr end;
char *buf_w, *buf_r;
size_t nbytes;

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		return -1;
	}
	geo = nvm_dev_get_geo(dev);

	nbytes = geo->sector_nbytes * geo->nsectors * geo->npages *
		geo->nplanes;

	buf_r = nvm_buf_alloc(geo, nbytes);
	if (!buf_r) {
		perror("nvm_buf_alloc");
		return -1;
	}
	buf_w = nvm_buf_alloc(geo, nbytes);
	if (!buf_w) {
		perror("nvm_buf_alloc");
		return -1;
	}
	nvm_buf_fill(buf_w, nbytes);

	bgn.ppa = end.ppa = 0;

	bgn.g.ch = ch_bgn;
	end.g.ch = ch_end;

	bgn.g.lun = lun_bgn;
	end.g.lun = lun_end;

	bgn.g.blk = blk;
	end.g.blk = blk;

	return 0;
}

int teardown(void)
{
	geo = NULL;
	nvm_dev_close(dev);
	free(buf_r);
	free(buf_w);

	return 0;
}

/**
 * Test that nvm_lba_pwrite / nvm_lba_pread works as expected.
 */
void test_VBLK_PE_LW_LR(void)
{
	for (int ch = bgn.g.ch; ch <= end.g.ch; ++ch) {
		for (int lun = bgn.g.lun; lun <= end.g.lun; ++lun) {
			for (int blk = bgn.g.blk; blk <= end.g.blk; ++blk) {
				struct nvm_vblk *vblk;
				struct nvm_addr addr = {.ppa=0};

				addr.g.ch = ch;
				addr.g.lun = lun;
				addr.g.blk = blk;

				vblk = nvm_vblk_alloc(dev, &addr, 1);
				nvm_vblk_erase(vblk);
				free(vblk);

				nvm_lba_pwrite(dev, buf_w, nbytes,
					       nvm_addr_gen2lba(dev, addr));
				nvm_lba_pread(dev, buf_r, nbytes,
					      nvm_addr_gen2lba(dev, addr));

				CU_ASSERT_NSTRING_EQUAL(buf_w, buf_r, nbytes);
			}
		}
	}
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 7:
		blk = atoi(argv[6]);
	case 6:
		lun_end = atoi(argv[5]);
	case 5:
		lun_end = atoi(argv[4]);
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

	pSuite = CU_add_suite("nvm_lba_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_lba_PE_LW_LR", test_VBLK_PE_LW_LR)) ||
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
