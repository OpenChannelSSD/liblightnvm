/*
 * test_cmd_copy.c - test ocssd vector copy
 *
 * Copyright (c) 2019 CNEX Labs, Inc.
 *
 * Written by Klaus Birkelund Abildgaard Jensen <klaus@birkelund.eu>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "test_util.h"
#include "test_intf.c"

static void cmd_copy_full(void)
{
	bool dulbe_state = nvm_test_set_dulbe(true);

	struct nvm_buf_set *bufs;
	struct nvm_addr addrs[2];
	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 2, addrs)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	bufs = nvm_buf_set_alloc(DEV, SECTOR_SIZE * NSECTR, OOB_SIZE * NSECTR);
	if (!bufs) {
		CU_FAIL("nvm_buf_set_alloc");
		return;
	}
	nvm_buf_set_fill(bufs);

	nvm_test_scalar_write_ok(addrs[0], NSECTR, bufs->write, bufs->write_meta);
	nvm_test_vector_copy_ok(addrs[0], addrs[1], NSECTR);
	nvm_test_scalar_read_ok(addrs[1], NSECTR, bufs->read, bufs->write,
		bufs->read_meta, bufs->write_meta);

	nvm_buf_set_free(bufs);

	if (nvm_cmd_erase(DEV, addrs, 2, NULL, 0x0, NULL)) {
		CU_FAIL("nvm_cmd_erase");
		return;
	}

	nvm_test_set_dulbe(dulbe_state);
}

static void cmd_copy_scatter(void)
{
	bool dulbe_state = nvm_test_set_dulbe(true);

	struct nvm_buf_set *bufs;
	struct nvm_addr chks[3];
	struct nvm_addr src_addrs[2*WS_MIN], dst_addrs[2*WS_MIN];

	bufs = nvm_buf_set_alloc(DEV, SECTOR_SIZE * NSECTR, OOB_SIZE * NSECTR);
	if (!bufs) {
		CU_FAIL_FATAL("nvm_buf_set_alloc");
		return;
	}
	nvm_buf_set_fill(bufs);

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 3, chks)) {
		CU_FAIL_FATAL("nvm_cmd_rprt_arbs");
		return;
	}

	nvm_test_scalar_write_ok(chks[0], NSECTR, bufs->write, bufs->write_meta);

	nvm_addr_fill_crange(src_addrs, chks[0], 2*WS_MIN);

	/* setup dst_addrs for scatter */
	nvm_addr_fill_crange(dst_addrs, chks[1], WS_MIN);
	nvm_addr_fill_crange(&dst_addrs[WS_MIN], chks[2], WS_MIN);

	if (nvm_cmd_copy(DEV, src_addrs, dst_addrs, 2*WS_MIN, 0, NULL)) {
		CU_FAIL_FATAL("nvm_cmd_copy");
	}

	nvm_test_pad_to_close(chks[1], 0x00);
	nvm_test_pad_to_close(chks[2], 0x00);

	nvm_test_scalar_read_ok(chks[1], WS_MIN, bufs->read, bufs->write,
		bufs->read_meta, bufs->write_meta);

	nvm_test_scalar_read_ok(chks[2], WS_MIN,
		&bufs->read[WS_MIN * SECTOR_SIZE], &bufs->write[WS_MIN * SECTOR_SIZE],
		&bufs->read_meta[WS_MIN * OOB_SIZE], &bufs->write_meta[WS_MIN * OOB_SIZE]);

	nvm_test_set_dulbe(dulbe_state);
}

static void cmd_copy_gather(void)
{
	bool dulbe_state = nvm_test_set_dulbe(true);

	struct nvm_buf_set *bufs;
	struct nvm_addr chks[3];
	struct nvm_addr src_addrs[2*WS_MIN], dst_addrs[2*WS_MIN];

	bufs = nvm_buf_set_alloc(DEV, SECTOR_SIZE * NSECTR, OOB_SIZE * NSECTR);
	if (!bufs) {
		CU_FAIL("nvm_buf_set_alloc");
		return;
	}
	nvm_buf_set_fill(bufs);

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 3, chks)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	nvm_test_scalar_write_ok(chks[0], NSECTR, bufs->write, bufs->write_meta);
	nvm_test_scalar_write_ok(chks[1], NSECTR, bufs->write, bufs->write_meta);

	/* setup src_addrs for gather */
	nvm_addr_fill_crange(src_addrs, chks[0], WS_MIN);
	nvm_addr_fill_crange(&src_addrs[WS_MIN], chks[1], WS_MIN);

	nvm_addr_fill_crange(dst_addrs, chks[2], 2*WS_MIN);

	if (nvm_cmd_copy(DEV, src_addrs, dst_addrs, 2*WS_MIN, 0, NULL)) {
		CU_FAIL("nvm_cmd_copy");
	}

	nvm_test_pad_to_close(chks[2], 0x00);

	nvm_test_scalar_read_ok(chks[2], WS_MIN, bufs->read, bufs->write,
		bufs->read_meta, bufs->write_meta);

	chks[2].l.sectr += WS_MIN;

	nvm_test_scalar_read_ok(chks[2], WS_MIN, bufs->read, bufs->write,
		bufs->read_meta, bufs->write_meta);

	nvm_test_set_dulbe(dulbe_state);
}


int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_cmd_copy", argc, argv, 0);
	if (!pSuite)
		goto out;

	if (!CU_ADD_TEST(pSuite, cmd_copy_full))
		goto out;
	if (!CU_ADD_TEST(pSuite, cmd_copy_scatter))
		goto out;
	if (!CU_ADD_TEST(pSuite, cmd_copy_gather))
		goto out;

	switch(RMODE) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(RMODE);
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
