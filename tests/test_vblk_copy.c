#include "test_intf.c"

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
		CU_FAIL("FAILED: Allocating nvm_buf_set");
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
		if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, naddrs, addrs))
			CU_FAIL("FAILED: nvm_cmd_rprt_arbs");
		break;
	}

	CU_ASSERT(!vblk_ewr(addrs, naddrs));
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_vblk_{erase,write,read}",
					argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "VBLK EWR S20", test_VBLK_EWR))
		goto out;

	switch(rmode) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(rmode);
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
