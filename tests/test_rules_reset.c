/*
 * test_rules_reset.c - verify reset access rules
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

static void _test_reset_free(nvm_test_reset_ok_fn reset_ok,
	nvm_test_reset_err_fn reset_err)
{
	struct nvm_addr addr;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	if (MULTIPLE_RESETS_SUPPORTED) {
		reset_ok(&addr);
	} else {
		reset_err(&addr, LNVM_ERR_INVALID_RESET);
	}
}

static void _test_reset_open(nvm_test_reset_ok_fn reset_ok,
	nvm_test_reset_err_fn reset_err)
{
	struct nvm_addr addr;
	char *buf;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	buf = nvm_buf_alloc(DEV, WS_MIN * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	nvm_test_scalar_write_ok(addr, WS_MIN, buf);

	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_OPEN);

	if (nvm_dev_get_mccap(DEV) & 0x4) {
		reset_ok(&addr);
	} else {
		reset_err(&addr, LNVM_ERR_INVALID_RESET);
	}
}

static void _test_reset_closed(nvm_test_reset_ok_fn reset_ok)
{
	struct nvm_addr addr;
	char *buf;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	buf = nvm_buf_alloc(DEV, GEO->l.nsectr * SECTOR_SIZE, NULL);
	CU_ASSERT_PTR_NOT_NULL(buf);

	nvm_test_scalar_write_ok(addr, GEO->l.nsectr, buf);

	nvm_test_rprt_assert_state(addr, NVM_CHUNK_STATE_CLOSED);

	reset_ok(&addr);
}

static void _test_reset_offline(nvm_test_reset_err_fn reset_err)
{
	struct nvm_addr addr;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_OFFLINE, 1, &addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		return;
	}

	reset_err(&addr, LNVM_ERR_OFFLINE_CHUNK);
}

static void _test_reset_non_existing(nvm_test_reset_err_fn reset_err)
{
	struct nvm_addr addr = {
		.l.chunk = GEO->l.nchunk,
	};

	reset_err(&addr, LNVM_ERR_INVALID_RESET);
}

MAKE_RULES_TESTS_2(reset_free, reset_ok, reset_err)
MAKE_RULES_TESTS_2(reset_open, reset_ok, reset_err)
MAKE_RULES_TESTS_1(reset_closed, reset_ok)
MAKE_RULES_TESTS_1(reset_offline, reset_err)
MAKE_RULES_TESTS_1(reset_non_existing, reset_err)

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_rules_reset", argc, argv, 0);
	if (!pSuite)
		goto out;

	/* SCALAR */
	if (!CU_add_test(pSuite, "reset rules: {state: FREE; mode: SCALAR}", test_scalar_reset_free))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: OPEN; mode: SCALAR}", test_scalar_reset_open))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: CLOSED; mode: SCALAR}", test_scalar_reset_closed))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: OFFLINE; mode: SCALAR}", test_scalar_reset_offline))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: N/A; mode: SCALAR}", test_scalar_reset_non_existing))
		goto out;

	/* VECTOR */
	if (!CU_add_test(pSuite, "reset rules: {state: FREE; mode: VECTOR}", test_vector_reset_free))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: OPEN; mode: VECTOR}", test_vector_reset_open))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: CLOSED; mode: VECTOR}", test_vector_reset_closed))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: OFFLINE; mode: VECTOR}", test_vector_reset_offline))
		goto out;
	if (!CU_add_test(pSuite, "reset rules: {state: N/A; mode: VECTOR}", test_vector_reset_non_existing))
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
