#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>
#include <liblightnvm.h>
#include <liblightnvm_spec.h>

#define NVM_TEST_RMODE_AUTO 0xA

static int rmode = CU_BRM_NORMAL;
static int seed = 0;
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
	
	srand(seed);

	return 0;
}

int teardown(void)
{
	geo = NULL;
	nvm_dev_close(dev);

	return 0;
}

int vblk_ewr(struct nvm_addr *addrs, int naddrs)
{
	struct nvm_buf_set *bufs = NULL;
	struct nvm_vblk *vblk = NULL;
	size_t nbytes = 0;

	if (CU_BRM_VERBOSE == rmode)
		nvm_addr_prn(addrs, naddrs, dev);

	vblk = nvm_vblk_alloc(dev, addrs, naddrs);
	if (!vblk) {
		CU_FAIL("FAILED: Allocating vblk");
		goto out;
	}
	nbytes = nvm_vblk_get_nbytes(vblk);

	bufs = nvm_buf_set_alloc(dev, nbytes, 0);
	if (!bufs) {
		goto out;
	}
	nvm_buf_set_fill(bufs);

	if (nvm_vblk_erase(vblk) < 0) {
		CU_FAIL("FAILED: nvm_vblk_erase");
		goto out;
	}

	if (nvm_vblk_write(vblk, bufs->write, nbytes) < 0) {
		CU_FAIL("FAILED: nvm_vblk_write");
		goto out;
	}

	if (nvm_vblk_read(vblk, bufs->read, nbytes) < 0) {
		CU_FAIL("FAILED: nvm_vblk_read");
		goto out;
	}

	if (nvm_buf_diff(bufs->write, bufs->read, nbytes)) {
		CU_FAIL("FAILED: nvm_buf_diff");
		goto out;
	}

out:
	nvm_vblk_free(vblk);
	nvm_buf_set_free(bufs);

	return 0;
}

void test_VBLK_EWR(void)
{
	struct nvm_addr addrs[0x1000] = { 0 };
	size_t naddrs = 0;

	switch(nvm_dev_get_verid(dev)) {
	case NVM_SPEC_VERID_12:
		naddrs = geo->g.nchannels * geo->g.nluns;
		if (nvm_cmd_gbbt_arbs(dev, NVM_BBT_FREE, naddrs, addrs))
			CU_FAIL("FAILED: nvm_cmd_gbbt_arbs");
		break;

	case NVM_SPEC_VERID_20:
		naddrs = geo->l.npugrp * geo->l.npunit;
		if (nvm_cmd_rprt_arbs(dev, NVM_RPRT_FREE, naddrs, addrs))
			CU_FAIL("FAILED: nvm_cmd_rprt_arbs");
		break;
	}

	CU_ASSERT(!vblk_ewr(addrs, naddrs));
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 4:
		switch(atoi(argv[3])) {
		case NVM_TEST_RMODE_AUTO:
			rmode = NVM_TEST_RMODE_AUTO;
			break;
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
		seed = atoi(argv[2]);
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return 1;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}

	if (!seed) {	// Default arbitrary seed
		seed = time(NULL);
	}

	printf("# TEST_INPUT: {dev: '%s', seed: %u, rmode: 0x%x}\n",
	       nvm_dev_path, seed, rmode);

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_vblk_{erase,write,read}*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "VBLK EWR S20", test_VBLK_EWR)) ||
	0)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	switch(rmode) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;
	default:
		/* Run all tests using the CUnit Basic interface */
		CU_basic_set_mode(rmode);
		CU_basic_run_tests();
		break;
	}
	CU_cleanup_registry();

	return CU_get_error();
}
