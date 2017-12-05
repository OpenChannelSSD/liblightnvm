#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

#define FLUSH_ALL 1

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static struct nvm_dev *dev;
static const struct nvm_geo *geo;

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
		return -1;
	}
	geo = nvm_dev_get_geo(dev);

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);

	return 0;
}

void test_CMD_RPRT_PUNIT(void)
{
	struct nvm_addr punit_addr = { .val=0 };
	struct nvm_addr chunk_addr = { .val=0 };
	struct nvm_spec_rprt *rprt[10] = { NULL }; // Array of pointers
	int rprt_cur = 0;
	struct nvm_vblk *vblk = NULL;
	struct nvm_ret ret = { 0 };
	ssize_t res = 0;

	// Construct an arbitrary punit address
	punit_addr.l.pugrp = geo->npugrp / 2;
	punit_addr.l.punit = geo->npunit / 2;

	// Test that we can retrieve chunk information for the given punit
	rprt[rprt_cur] = nvm_cmd_rprt(dev, &punit_addr, 0x0, &ret);
	CU_ASSERT_PTR_NOT_NULL(rprt[rprt_cur]);
	if (!rprt[rprt_cur])
		goto out;

	// Test that it reports the correct amount of chunks
	assert(rprt[rprt_cur]->nchunks == geo->nchunk);

	// Get an arbitrary non-offline chunk in the punit
	chunk_addr.val = punit_addr.val;
	//for (size_t idx = geo->nchunk / 2; idx < geo->nchunk; ++idx) {
	for (size_t idx = 0; idx < geo->nchunk; ++idx) {
		if (rprt[rprt_cur]->descr[idx].chunk_state == NVM_RPRT_OFFLINE)
			continue;

		chunk_addr.l.chunk = idx;
		break;
	}

	// TODO: check the address...

	// Create a vblk for the address
	vblk = nvm_vblk_alloc(dev, &chunk_addr, 1);
	CU_ASSERT_PTR_NOT_NULL(vblk);
	if (!vblk)
		goto out;

	// Erase it and check that it is in state FREE
	res = nvm_vblk_erase(vblk);
	CU_ASSERT(res >= 0);
	if (res < 0)
		goto out;

	rprt[++rprt_cur] = nvm_cmd_rprt(dev, &punit_addr, 0x0, &ret);
	CU_ASSERT_PTR_NOT_NULL(rprt[rprt_cur]);
	if (!rprt[rprt_cur])
		goto out;

	CU_ASSERT(rprt[rprt_cur]->descr[chunk_addr.l.chunk].chunk_state == NVM_RPRT_FREE);

	nvm_addr_print(&chunk_addr, 1, dev);
	nvm_vblk_pr(vblk);

	// Write it and check that it changed state to closed
	res = nvm_vblk_pad(vblk);
	CU_ASSERT(res >= 0);
	if (res < 0)
		goto out;
	
	rprt[++rprt_cur] = nvm_cmd_rprt(dev, &punit_addr, 0x0, &ret);
	CU_ASSERT_PTR_NOT_NULL(rprt[rprt_cur]);
	if (!rprt[rprt_cur])
		goto out;

	CU_ASSERT(rprt[rprt_cur]->descr[chunk_addr.l.chunk].chunk_state == NVM_RPRT_CLOSED);

out:
	nvm_vblk_free(vblk);
	for (int idx = 0; idx <= rprt_cur; ++idx)
		nvm_buf_free(rprt[idx]);
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

	pSuite = CU_add_suite("nvm_cmd_rprt*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_cmd_rprt_punit", test_CMD_RPRT_PUNIT)) ||
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
