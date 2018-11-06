/*
 * test_rules_write.c - verify write access rules
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
#include "test_rules.h"
#include "test_intf.c"

#include <CUnit/Basic.h>

static struct nvm_addr addr;

static void _test_write_slba(nvm_test_write_ok_fn write_ok)
{
	char *buf;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	buf = nvm_buf_alloc(DEV, WS_MIN * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	write_ok(addr, WS_MIN, buf);

	nvm_test_rprt_assert_wp(addr, WS_MIN);
	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	nvm_buf_free(DEV, buf);
}

static void _test_write_wp(nvm_test_write_ok_fn write_ok)
{
	struct nvm_spec_rprt *rprt;
	uint32_t wp;
	char *buf;

	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	rprt = nvm_test_rprt(addr);
	wp = rprt->descr[addr.l.chunk].wp;
	nvm_buf_free(DEV, rprt);

	addr.l.sectr = wp;

	buf = nvm_buf_alloc(DEV, WS_MIN * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	write_ok(addr, WS_MIN, buf);

	nvm_test_rprt_assert_wp(addr, wp + WS_MIN);
	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	nvm_buf_free(DEV, buf);
}

static void _test_write_ooo(nvm_test_write_err_fn write_err)
{
	struct nvm_spec_rprt *rprt;
	uint32_t wp;
	char *buf;

	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	rprt = nvm_test_rprt(addr);
	wp = rprt->descr[addr.l.chunk].wp;
	nvm_buf_free(DEV, rprt);

	// write at wp+ws_min (out of order)
	addr.l.sectr = wp + WS_MIN;

	buf = nvm_buf_alloc(DEV, WS_MIN * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	write_err(addr, WS_MIN, buf, LNVM_ERR_OUT_OF_ORDER);

	nvm_test_rprt_assert_wp(addr, wp);
	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	// write at wp-ws_min (out of order)
	addr.l.sectr = wp - WS_MIN;

	write_err(addr, WS_MIN, buf, LNVM_ERR_OUT_OF_ORDER);

	nvm_test_rprt_assert_wp(addr, wp);
	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	nvm_buf_free(DEV, buf);
}

static void _test_write_wp_close(nvm_test_write_ok_fn write_ok)
{
	struct nvm_spec_rprt *rprt;
	uint32_t wp, nlba;
	char *buf;

	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	rprt = nvm_test_rprt(addr);
	wp = rprt->descr[addr.l.chunk].wp;
	nlba = rprt->descr[addr.l.chunk].naddrs - wp;
	nvm_buf_free(DEV, rprt);

	addr.l.sectr = wp;

	buf = nvm_buf_alloc(DEV, nlba * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	write_ok(addr, nlba, buf);

	nvm_test_rprt_assert_wp(addr, nlba + wp);
	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_CLOSED);

	nvm_buf_free(DEV, buf);
}

static void _test_write_wp_closed(nvm_test_write_err_fn write_err)
{
	char *buf;

	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_CLOSED);

	addr.l.sectr = 0;

	buf = nvm_buf_alloc(DEV, WS_MIN * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	write_err(addr, WS_MIN, buf, NVME_ERR_WRITE_FAULT);

	nvm_buf_free(DEV, buf);
}

static void _test_write_offline(nvm_test_write_err_fn write_err)
{
	char *buf;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_OFFLINE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	buf = nvm_buf_alloc(DEV, WS_MIN * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	write_err(addr, WS_MIN, buf, NVME_ERR_WRITE_FAULT);

	nvm_buf_free(DEV, buf);
}

static void _test_write_oor(nvm_test_write_err_fn write_err)
{
	char *buf = nvm_buf_alloc(DEV, WS_MIN * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	// make out of range address
	addr.l.pugrp = GEO->l.npugrp;

	write_err(addr, WS_MIN, buf, NVME_ERR_WRITE_FAULT);

	nvm_buf_free(DEV, buf);
}

MAKE_RULES_TESTS_1(write_slba, write_ok)
MAKE_RULES_TESTS_1(write_wp, write_ok)
MAKE_RULES_TESTS_1(write_ooo, write_err)
MAKE_RULES_TESTS_1(write_wp_close, write_ok)
MAKE_RULES_TESTS_1(write_wp_closed, write_err)
MAKE_RULES_TESTS_1(write_offline, write_err)
MAKE_RULES_TESTS_1(write_oor, write_err)

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_rules_write",
					argc, argv);
	if (!pSuite)
		goto out;

	// DO NOT REORDER; some of these tests reuse LBAs!
	if (!CU_add_test(pSuite, "write rules: {state: FREE; mode: SCALAR; write at SLBA}", test_scalar_write_slba))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OPEN; mode: SCALAR; write at WP}", test_scalar_write_wp))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OPEN; mode: SCALAR; write out of order}", test_scalar_write_ooo))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OPEN; mode: SCALAR; write at WP such that state => CLOSED}", test_scalar_write_wp_close))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: CLOSED; mode: SCALAR; write at SLBA}", test_scalar_write_wp_closed))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OFFLINE; mode: SCALAR}", test_scalar_write_offline))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: N/A; mode: SCALAR; write to non-existing LBA}", test_scalar_write_oor))
		goto out;

	if (!CU_add_test(pSuite, "write rules: {state: FREE; mode: VECTOR; write at SLBA}", test_vector_write_slba))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OPEN; mode: VECTOR; write at WP}", test_vector_write_wp))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OPEN; mode: VECTOR; write out of order}", test_vector_write_ooo))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OPEN; mode: VECTOR; write at WP such that state => CLOSED}", test_vector_write_wp_close))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: CLOSED; mode: VECTOR; write at SLBA}", test_vector_write_wp_closed))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: OFFLINE; mode: VECTOR}", test_vector_write_offline))
		goto out;
	if (!CU_add_test(pSuite, "write rules: {state: N/A; mode: VECTOR; write to non-existing LBA}", test_vector_write_oor))
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
