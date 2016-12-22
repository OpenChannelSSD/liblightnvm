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
static const struct nvm_geo *dev_geo, *vblk_geo;
struct nvm_vblk *vblk;
char *buf_w, *buf_r;

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	CU_ASSERT_PTR_NOT_NULL(dev);
	if (!dev) {
		perror("nvm_dev_open");
		return -1;
	}
	dev_geo = nvm_dev_attr_geo(dev);

	vblk = nvm_vblk_alloc_span(dev, ch_bgn, ch_end, lun_bgn, lun_end, blk);
	if (!vblk) {
		perror("nvm_vblk_alloc_span");
		return -1;
	}
	vblk_geo = nvm_vblk_attr_geo(vblk);

	buf_w = nvm_buf_alloc(dev_geo, vblk_geo->tbytes);
	if (!buf_w) {
		perror("nvm_buf_alloc");
		return -1;
	}

	buf_r = nvm_buf_alloc(dev_geo, vblk_geo->tbytes);
	if (!buf_r) {
		perror("nvm_buf_alloc");
		return -1;
	}

	return 0;
}

int teardown(void)
{
	dev_geo = NULL;
	nvm_vblk_free(vblk);
	nvm_dev_close(dev);
	free(buf_r);
	free(buf_w);

	return 0;
}

void test_VBLK_ERWR(void)
{
	ssize_t res = 0;

	res = nvm_vblk_erase(vblk);				// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: Erasing vblk");
	}

	res = nvm_vblk_read(vblk, buf_r, vblk_geo->tbytes);	// EXPECT: Fail
	CU_ASSERT(res < 0);
	if (res >=  0) {
		CU_FAIL("FAILED: nvm_vblk_read OK of NON-written vblk -> should fail");
		return;
	}
	
	res = nvm_vblk_write(vblk, buf_w, vblk_geo->tbytes);	// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: nvm_vblk_write");
		return;
	}

	res = nvm_vblk_read(vblk, buf_w, vblk_geo->tbytes);	// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: nvm_vblk_write");
		return;
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

	pSuite = CU_add_suite("nvm_bbt_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_vblk_erwr", test_VBLK_ERWR)) ||
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
