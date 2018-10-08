/*
 * test_rules_read - verify read access rules
 *
 * Copyright (C) Klaus B. A. Jensen <klaus.jensen@cnexlabs.com>
 * All rights reserved.
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

typedef void (*_read_and_verify_fn)(struct nvm_addr *addr, int dulbe,
	nvm_test_verify_fn verify, uint16_t err);

static void _test_read_free(_read_and_verify_fn read_and_verify)
{
	struct nvm_addr addr;
	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	read_and_verify(&addr, false /* dulbe */,
		nvm_test_verify_read_ok_predef, 0x0);
}

static void _test_read_free_dulbe(_read_and_verify_fn read_and_verify)
{
	struct nvm_addr addr;
	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	read_and_verify(&addr, true /* dulbe */,
		nvm_test_verify_read_err, NVME_ERR_DULB);
}

static void _test_read_offline(_read_and_verify_fn read_and_verify)
{
	struct nvm_addr addr;
	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_OFFLINE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	read_and_verify(&addr, false /* dulbe */,
		nvm_test_verify_read_ok_predef, 0x0);
}

static void _test_read_offline_dulbe(_read_and_verify_fn read_and_verify)
{
	struct nvm_addr addr;
	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_OFFLINE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	read_and_verify(&addr, true /* dulbe */,
		nvm_test_verify_read_err, NVME_ERR_DULB);
}

static void _test_read_oor(_read_and_verify_fn read_and_verify)
{
	nvm_test_set_dulbe(false);

	struct nvm_addr addr = {
		.l.chunk = geo->l.nchunk,
	};

	read_and_verify(&addr, false /* dulbe */,
		nvm_test_verify_read_ok_predef, 0x0);
}

static void _test_read_oor_dulbe(_read_and_verify_fn read_and_verify)
{
	nvm_test_set_dulbe(true);

	struct nvm_addr addr = {
		.l.chunk = geo->l.nchunk,
	};

	read_and_verify(&addr, true /* dulbe */,
		nvm_test_verify_read_err, NVME_ERR_DULB);
}

static void _test_read_mw_cunits(nvm_test_write_ok_fn write_ok,
	nvm_test_read_ok_fn read_ok,
	nvm_test_read_ok_predef_fn read_ok_predef)
{
	struct nvm_addr addr;
	struct nvm_buf_set *bufs;

	nvm_test_set_dulbe(false);

	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	bufs = nvm_buf_set_alloc(dev, geo->l.nsectr * SECTOR_SIZE, 0);
	if (bufs == NULL) {
		CU_FAIL("nvm_buf_set_alloc");
		return;
	}

	nvm_buf_set_fill(bufs);

	// prefill read buffer with something else than NVME_DLFEAT_VAL
	memset(bufs->read, NVME_DLFEAT_VAL+1, bufs->nbytes);

	// write MW_CUNITS LBAs
	write_ok(addr, MW_CUNITS, bufs->write);

	// read and verify that predefined data is returned
	read_ok_predef(addr, MW_CUNITS, bufs->read);

	// reset read buffer with something else than NVME_DLFEAT_VAL
	memset(bufs->read, NVME_DLFEAT_VAL+1, bufs->nbytes);

	// append WS_MIN LBAs to chunk
	addr.l.sectr = MW_CUNITS;
	write_ok(addr, WS_MIN, bufs->write);

	// read from WS_MIN LBAs at sector 0 and verify that previously written
	// data is returned.
	addr.l.sectr = 0;
	read_ok(addr, WS_MIN, bufs->read, bufs->write);

	nvm_buf_set_free(bufs);
}

static void _test_read_mw_cunits_dulbe(nvm_test_write_ok_fn write_ok,
	nvm_test_read_err_fn read_err,
	nvm_test_read_ok_fn read_ok)
{
	struct nvm_addr addr;
	struct nvm_buf_set *bufs;

	nvm_test_set_dulbe(true);

	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	bufs = nvm_buf_set_alloc(dev, geo->l.nsectr * SECTOR_SIZE, 0);
	if (bufs == NULL) {
		CU_FAIL("nvm_buf_set_alloc");
		return;
	}

	nvm_buf_set_fill(bufs);

	// write MW_CUNITS LBAs
	write_ok(addr, MW_CUNITS, bufs->write);

	// read and verify that error is returned
	read_err(addr, MW_CUNITS, bufs->read, NVME_ERR_DULB);

	// append WS_MIN LBAs to chunk
	addr.l.sectr = MW_CUNITS;
	write_ok(addr, WS_MIN, bufs->write);

	// read from WS_MIN LBAs at sector 0 and verify that previously written
	// data is returned.
	addr.l.sectr = 0;
	read_ok(addr, WS_MIN, bufs->read, bufs->write);

	nvm_buf_set_free(bufs);
}

static void _test_read_slba_gt_wp(nvm_test_write_ok_fn write_ok,
	nvm_test_read_ok_predef_fn read_ok_predef)
{
	struct nvm_addr addr;
	struct nvm_buf_set *bufs;

	nvm_test_set_dulbe(false);

	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	bufs = nvm_buf_set_alloc(dev, geo->l.nsectr * SECTOR_SIZE, 0);
	if (bufs == NULL) {
		CU_FAIL("nvm_buf_set_alloc");
		return;
	}

	nvm_buf_set_fill(bufs);

	// prefill read buffer with something else than NVME_DLFEAT_VAL
	memset(bufs->read, NVME_DLFEAT_VAL+1, bufs->nbytes);

	// write MW_CUNITS+WS_MIN LBAs
	write_ok(addr, MW_CUNITS + WS_MIN, bufs->write);

	// verify that WP is at MW_CUNITS+WS_MIN
	nvm_test_rprt_assert_wp(addr, MW_CUNITS + WS_MIN);

	addr.l.sectr = MW_CUNITS + WS_MIN;

	// read WS_MIN after WP and verify that predefined data is returned
	read_ok_predef(addr, WS_MIN, bufs->read);

	nvm_buf_set_free(bufs);
}

static void _test_read_slba_gt_wp_dulbe(nvm_test_write_ok_fn write_ok,
	nvm_test_read_err_fn read_err)
{
	struct nvm_addr addr;
	struct nvm_buf_set *bufs;

	nvm_test_set_dulbe(true);

	if (nvm_cmd_rprt_arbs(dev, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	bufs = nvm_buf_set_alloc(dev, geo->l.nsectr * SECTOR_SIZE, 0);
	if (bufs == NULL) {
		CU_FAIL("nvm_buf_set_alloc");
		return;
	}

	nvm_buf_set_fill(bufs);

	// prefill read buffer with something else than NVME_DLFEAT_VAL
	memset(bufs->read, NVME_DLFEAT_VAL+1, bufs->nbytes);

	// write MW_CUNITS+WS_MIN LBAs
	write_ok(addr, MW_CUNITS + WS_MIN, bufs->write);

	// verify that WP is at MW_CUNITS+WS_MIN
	nvm_test_rprt_assert_wp(addr, MW_CUNITS + WS_MIN);

	addr.l.sectr = MW_CUNITS + WS_MIN;

	// read WS_MIN after WP and verify that error is returned
	read_err(addr, WS_MIN, bufs->read, NVME_ERR_DULB);

	nvm_buf_set_free(bufs);
}

MAKE_TESTS_1(read_free, read_and_verify)
MAKE_TESTS_1(read_free_dulbe, read_and_verify)
MAKE_TESTS_1(read_offline, read_and_verify)
MAKE_TESTS_1(read_offline_dulbe, read_and_verify)
MAKE_TESTS_1(read_oor, read_and_verify)
MAKE_TESTS_1(read_oor_dulbe, read_and_verify)
MAKE_TESTS_3(read_mw_cunits, write_ok, read_ok, read_ok_predef)
MAKE_TESTS_3(read_mw_cunits_dulbe, write_ok, read_err, read_ok)
MAKE_TESTS_2(read_slba_gt_wp, write_ok, read_ok_predef)
MAKE_TESTS_2(read_slba_gt_wp_dulbe, write_ok, read_err)

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_rules_read",
					argc, argv);
	if (!pSuite)
		goto out;

	/* SCALAR */
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: SCALAR; dulbe: OFF}", test_scalar_read_free))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: SCALAR; dulbe: ON}", test_scalar_read_free_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OFFLINE; mode: SCALAR; dulbe: OFF}", test_scalar_read_offline))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OFFLINE; mode: SCALAR; dulbe: ON}", test_scalar_read_offline_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: N/A; mode: SCALAR; non-existing LBA; dulbe: OFF}", test_scalar_read_oor))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: N/A; mode: SCALAR; non-existing LBA; dulbe: ON}", test_scalar_read_oor_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OPEN; mode: SCALAR; check/violate MW_CUNITS; dulbe: OFF}", test_scalar_read_mw_cunits))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OPEN; mode: SCALAR; check/violate MW_CUNITS; dulbe: ON}", test_scalar_read_mw_cunits_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: SCALAR; slba > wp; dulbe: OFF}", test_scalar_read_slba_gt_wp))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: SCALAR; slba > wp; dulbe: ON}", test_scalar_read_slba_gt_wp_dulbe))
		goto out;

	/* VECTOR */
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: VECTOR; dulbe: OFF}", test_vector_read_free))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: VECTOR; dulbe: ON}", test_vector_read_free_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OFFLINE; mode: VECTOR; dulbe: OFF}", test_vector_read_offline))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OFFLINE; mode: VECTOR; dulbe: ON}", test_vector_read_offline_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: N/A; mode: VECTOR; non-existing LBA; dulbe: OFF}", test_vector_read_oor))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: N/A; mode: VECTOR; non-existing LBA; dulbe: ON}", test_vector_read_oor_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OPEN; mode: VECTOR; check/violate MW_CUNITS; dulbe: OFF}", test_vector_read_mw_cunits))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: OPEN; mode: VECTOR; check/violate MW_CUNITS; dulbe: ON}", test_vector_read_mw_cunits_dulbe))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: VECTOR; slba > wp; dulbe: OFF}", test_vector_read_slba_gt_wp))
		goto out;
	if (!CU_add_test(pSuite, "read rules: {state: FREE; mode: VECTOR; slba > wp; dulbe: ON}", test_vector_read_slba_gt_wp_dulbe))
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
