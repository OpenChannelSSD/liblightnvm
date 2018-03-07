#include "test_intf.c"

static size_t descr_idx(struct nvm_addr *punit_addr, struct nvm_addr chunk_addr)
{
	if (punit_addr)
		return chunk_addr.l.chunk;

	return nvm_addr_gen2lpo(dev, chunk_addr) / sizeof(struct nvm_spec_rprt_descr);
}

void cmd_rprt(struct nvm_addr *punit_addr)
{
	struct nvm_addr chunk_addr = { .val=0 };
	struct nvm_spec_rprt *rprt[10] = { NULL }; ///> Report pointers
	int rprt_cur = 0;
	struct nvm_vblk *vblk = NULL;
	size_t buf_len = 0;
	char *buf = NULL;
	struct nvm_ret ret = { 0 };
	ssize_t res = 0;

	size_t tchunks = punit_addr ? geo->l.nchunk : geo->l.npugrp * geo->l.npunit * geo->l.nchunk;

	// Test that we can retrieve chunk information for the given punit
	rprt[rprt_cur] = nvm_cmd_rprt(dev, punit_addr, 0x0, &ret);
	CU_ASSERT_PTR_NOT_NULL(rprt[rprt_cur]);
	if (!rprt[rprt_cur])
		goto out;

	// Test that it reports the correct amount of chunks
	if (rprt[rprt_cur]->ndescr != tchunks) {
		CU_ASSERT(rprt[rprt_cur]->ndescr == tchunks);
		goto out;
	}

	// Get an arbitrary free chunk
	if (punit_addr) {
		chunk_addr.val = punit_addr->val;
	} else {
		chunk_addr.l.pugrp = geo->l.npugrp / 2;
		chunk_addr.l.punit = geo->l.npugrp / 2;
	}
	for (size_t idx = geo->l.nchunk / 2; idx < geo->l.nchunk; ++idx) {
		chunk_addr.l.chunk = idx;

		if (rprt[rprt_cur]->descr[descr_idx(punit_addr, chunk_addr)].cs == NVM_CHUNK_STATE_FREE)
			break;
	}

	// Test that the address found is valid
	res = nvm_addr_check(chunk_addr, dev);
	CU_ASSERT(!res);
	if (res)
		goto out;

	// Create a vblk for the address
	vblk = nvm_vblk_alloc(dev, &chunk_addr, 1);
	CU_ASSERT_PTR_NOT_NULL(vblk);
	if (!vblk)
		goto out;

	// Write it partially and check that it changed state to OPEN and wp
	{
		buf_len = nvm_vblk_get_nbytes(vblk) / 2;
		buf = nvm_buf_alloc(geo, buf_len);

		res = nvm_vblk_write(vblk, buf, buf_len);
		CU_ASSERT(res >= 0);
		if (res < 0)
			goto out;
		
		rprt[++rprt_cur] = nvm_cmd_rprt(dev, punit_addr, 0x0, &ret);
		CU_ASSERT_PTR_NOT_NULL(rprt[rprt_cur]);
		if (!rprt[rprt_cur])
			goto out;

		CU_ASSERT(rprt[rprt_cur]->descr[descr_idx(punit_addr, chunk_addr)].cs == NVM_CHUNK_STATE_OPEN);
		CU_ASSERT(rprt[rprt_cur]->descr[descr_idx(punit_addr, chunk_addr)].wp == (buf_len / geo->l.nbytes));
	}

	// Write it fully and check that it changed state to CLOSED and wp
	{
		res = nvm_vblk_pad(vblk);
		CU_ASSERT(res >= 0);
		if (res < 0)
			goto out;
		
		rprt[++rprt_cur] = nvm_cmd_rprt(dev, punit_addr, 0x0, &ret);
		CU_ASSERT_PTR_NOT_NULL(rprt[rprt_cur]);
		if (!rprt[rprt_cur])
			goto out;

		CU_ASSERT(rprt[rprt_cur]->descr[descr_idx(punit_addr, chunk_addr)].cs == NVM_CHUNK_STATE_CLOSED);
		CU_ASSERT(rprt[rprt_cur]->descr[descr_idx(punit_addr, chunk_addr)].wp == geo->l.nsectr);
	}

	// Erase it and check that it is in state FREE
	{
		res = nvm_vblk_erase(vblk);
		CU_ASSERT(res >= 0);
		if (res < 0)
			goto out;

		rprt[++rprt_cur] = nvm_cmd_rprt(dev, punit_addr, 0x0, &ret);
		CU_ASSERT_PTR_NOT_NULL(rprt[rprt_cur]);
		if (!rprt[rprt_cur])
			goto out;

		CU_ASSERT(rprt[rprt_cur]->descr[descr_idx(punit_addr, chunk_addr)].cs == NVM_CHUNK_STATE_FREE);
		CU_ASSERT(rprt[rprt_cur]->descr[descr_idx(punit_addr, chunk_addr)].wp == 0);
	}

out:
	nvm_vblk_free(vblk);
	for (int idx = 0; idx <= rprt_cur; ++idx)
		nvm_buf_free(rprt[idx]);
	nvm_buf_free(buf);
}

void test_CMD_RPRT_PUNIT(void)
{
	struct nvm_addr punit_addr = { .val=0 };

	// Construct an arbitrary punit address
	punit_addr.l.pugrp = geo->l.npugrp / 2;
	punit_addr.l.punit = geo->l.npunit / 2;

	cmd_rprt(&punit_addr);
}

void test_CMD_RPRT_ALL(void)
{
	cmd_rprt(NULL);
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_cmd_rprt_*", argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "nvm_cmd_rprt_punit", test_CMD_RPRT_PUNIT))
		goto out;
	if (!CU_add_test(pSuite, "nvm_cmd_rprt_all", test_CMD_RPRT_ALL))
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
