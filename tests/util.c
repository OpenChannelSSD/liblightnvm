/*
 * util.c - various utility functions for use when writing tests.
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

#include <CUnit/Basic.h>

bool nvm_test_set_dulbe(bool enable)
{
	union nvm_nvme_feat feat = { 0 };
	int rc;
	bool prev_state;

	rc = nvm_cmd_gfeat(DEV, NVM_NVME_FEAT_ERROR_RECOVERY, &feat, NULL);
	CU_ASSERT(rc == 0);

	prev_state = feat.error_recovery.dulbe;

	memset(&feat, 0x0, sizeof(union nvm_nvme_feat));

	if (enable) {
		feat.error_recovery.dulbe = 1;
	}

	rc = nvm_cmd_sfeat(DEV, NVM_NVME_FEAT_ERROR_RECOVERY, &feat, NULL);
	CU_ASSERT(rc == 0);

	return prev_state;
}

void nvm_test_read_and_verify(struct nvm_addr *addr, bool dulbe,
			      enum nvm_cmd_opts cmd_opts,
			      nvm_test_verify_fn verify, uint16_t err)
{
	bool dulbe_state = nvm_test_set_dulbe(dulbe);
	void *buf;
	struct nvm_ret ret = { 0 };
	const char v = NVME_DLFEAT_VAL;
	int rc;

	buf = nvm_buf_alloc(DEV, GEO->l.nbytes, NULL);
	if (buf == NULL) {
		nvm_test_set_dulbe(dulbe_state);
		CU_FAIL_FATAL("nvm_buf_alloc");
		return;
	}

	nvm_buf_fill(buf, GEO->l.nbytes);

	rc = nvm_cmd_read(DEV, addr, 1, buf, NULL, cmd_opts, &ret);

	verify(rc, &ret, buf, &v, GEO->l.nbytes, err);

	nvm_buf_free(DEV, buf);

	nvm_test_set_dulbe(dulbe_state);
	CU_PASS("SUCCESS");
}

void nvm_test_scalar_read_and_verify(struct nvm_addr *addr, bool dulbe,
				     nvm_test_verify_fn verify, uint16_t err)
{
	nvm_test_read_and_verify(addr, dulbe, NVM_CMD_SCALAR, verify, err);
}

void nvm_test_vector_read_and_verify(struct nvm_addr *addr, bool dulbe,
				     nvm_test_verify_fn verify, uint16_t err)
{
	nvm_test_read_and_verify(addr, dulbe, NVM_CMD_VECTOR, verify, err);
}

void nvm_test_verify_read_ok_predef(int rc,
				    const struct nvm_ret *NVM_UNUSED(ret),
				    const void *buf,
				    const void *NVM_UNUSED(expected),
				    size_t count, uint16_t NVM_UNUSED(err))
{
	const uint8_t *ubuf = buf;

	uint8_t dlfeat_val;
	size_t diff = 0;

	CU_ASSERT(rc == 0);

	switch (NS->dlfeat & 0x7) {
		case 0x1:
			dlfeat_val = 0x00;
			break;
		case 0x2:
			dlfeat_val = 0xff;
			break;
		case 0x0:
			/* value of deallocated or unwritten blocks are not
			 * reported, so do not check the buffer */

			/* fallthrough */
		default:
			return;
	}

	for (size_t i = 0; i < count; i++) {
		if (ubuf[i] == dlfeat_val) {
			continue;
		}

		++diff;
	}

	CU_ASSERT(diff == 0);

	if (diff && CU_BRM_VERBOSE == RMODE) {
		nvm_buf_pr(buf, count);
	}
}

void nvm_test_verify_read_ok(int rc, const struct nvm_ret *NVM_UNUSED(ret),
			     const void *buf, const void *expected,
			     size_t count, uint16_t NVM_UNUSED(err))
{
	CU_ASSERT(rc == 0);
	CU_ASSERT(nvm_buf_diff(buf, expected, count) == 0);
}

void nvm_test_verify_err(int rc, const struct nvm_ret *ret, uint16_t err)
{
	uint8_t sc = NVM_TEST_NVME_STATUS_SC(ret->status);
	uint8_t sc_want = NVM_TEST_NVME_STATUS_SC(err);
	uint8_t sct = NVM_TEST_NVME_STATUS_SCT(ret->status);
	uint8_t sct_want = NVM_TEST_NVME_STATUS_SCT(err);

	CU_ASSERT(rc == -1);
	CU_ASSERT(sc == sc_want);
	CU_ASSERT(sct == sct_want);

	if (CU_BRM_VERBOSE == RMODE) {
		nvm_ret_pr(ret);

		if (rc != -1) {
			fprintf(stderr, "expected `rc == -1`; got %d\n", rc);
		}

		if (sc != sc_want) {
			fprintf(stderr, "expected `sc == 0x%x; got 0x%x\n",
				sc_want, sc);
		}

		if (sct != sct_want) {
			fprintf(stderr, "expected `sct == 0x%x; got 0x%x\n",
				sct_want, sct);
		}
	}
}

void nvm_test_verify_read_err(int rc, const struct nvm_ret *ret,
			      const void *NVM_UNUSED(buf),
			      const void *NVM_UNUSED(expected),
			      size_t NVM_UNUSED(count), uint16_t err)
{
	nvm_test_verify_err(rc, ret, err);
}

void nvm_test_read_oneshot_ok(struct nvm_addr *addr, uint32_t nlba, void *buf,
			      const void *expected, enum nvm_cmd_opts cmd_opts)
{
	int rc = nvm_cmd_read(DEV, addr, nlba, buf, NULL, cmd_opts, NULL);
	nvm_test_verify_read_ok(rc, NULL, buf, expected, nlba * SECTOR_SIZE,
				0x0);
}

void nvm_test_scalar_read_oneshot_ok(struct nvm_addr addr, uint32_t nlba,
				     void *buf, const void *expected)
{
	nvm_test_read_oneshot_ok(&addr, nlba, buf, expected, NVM_CMD_SCALAR);
}

void nvm_test_vector_read_oneshot_ok(struct nvm_addr slba, uint32_t nlba,
				     void *buf, const void *expected)
{
	struct nvm_addr addrs[nlba];
	nvm_addr_fill_crange(addrs, slba, nlba);
	nvm_test_read_oneshot_ok(addrs, nlba, buf, expected, NVM_CMD_VECTOR);
}

void nvm_test_write_oneshot_ok(struct nvm_addr *addr, uint32_t nlba,
			       const void *buf, enum nvm_cmd_opts cmd_opts)
{
	int rc = nvm_cmd_write(DEV, addr, nlba, buf, NULL, cmd_opts, NULL);
	CU_ASSERT(rc == 0);
}

void nvm_test_scalar_write_oneshot_ok(struct nvm_addr addr, uint32_t nlba,
				      const void *buf)
{
	nvm_test_write_oneshot_ok(&addr, nlba, buf, NVM_CMD_SCALAR);
}

void nvm_test_vector_write_oneshot_ok(struct nvm_addr slba, uint32_t nlba,
				      const void *buf)
{
	struct nvm_addr addrs[nlba];
	nvm_addr_fill_crange(addrs, slba, nlba);
	nvm_test_write_oneshot_ok(addrs, nlba, buf, NVM_CMD_VECTOR);
}

void nvm_test_read_oneshot_ok_predef(struct nvm_addr *addr, uint32_t nlba,
				     void *buf, enum nvm_cmd_opts cmd_opts)
{
	int rc = nvm_cmd_read(DEV, addr, nlba, buf, NULL, cmd_opts, NULL);
	nvm_test_verify_read_ok_predef(rc, NULL, buf, NULL, nlba * SECTOR_SIZE,
				       0x0);
}

void nvm_test_scalar_read_oneshot_ok_predef(struct nvm_addr addr, uint32_t nlba,
					    void *buf)
{
	nvm_test_read_oneshot_ok_predef(&addr, nlba, buf, NVM_CMD_SCALAR);
}

void nvm_test_vector_read_oneshot_ok_predef(struct nvm_addr slba, uint32_t nlba,
					    void *buf)
{
	struct nvm_addr addrs[nlba];
	nvm_addr_fill_crange(addrs, slba, nlba);
	nvm_test_read_oneshot_ok_predef(addrs, nlba, buf, NVM_CMD_VECTOR);
}

void nvm_test_read_oneshot_err(struct nvm_addr *addr, uint32_t nlba, void *buf,
			       enum nvm_cmd_opts cmd_opts, uint16_t err)
{
	struct nvm_ret ret = { 0 };
	int rc = nvm_cmd_read(DEV, addr, nlba, buf, NULL, cmd_opts, &ret);
	nvm_test_verify_read_err(rc, &ret, NULL, NULL, 0, err);
}

void nvm_test_scalar_read_oneshot_err(struct nvm_addr addr, uint32_t nlba,
				      void *buf, uint16_t err)
{
	nvm_test_read_oneshot_err(&addr, nlba, buf, NVM_CMD_SCALAR, err);
}

void nvm_test_vector_read_oneshot_err(struct nvm_addr slba, uint32_t nlba,
				      void *buf, uint16_t err)
{
	struct nvm_addr addrs[nlba];
	nvm_addr_fill_crange(addrs, slba, nlba);
	nvm_test_read_oneshot_err(addrs, nlba, buf, NVM_CMD_VECTOR, err);
}

void nvm_test_write_oneshot_err(struct nvm_addr *addr, uint32_t nlba,
				const void *buf, enum nvm_cmd_opts cmd_opts,
				uint32_t err)
{
	struct nvm_ret ret = { 0 };
	int rc = nvm_cmd_write(DEV, addr, nlba, buf, NULL, cmd_opts, &ret);

	nvm_test_verify_err(rc, &ret, err);
}

void nvm_test_scalar_write_oneshot_err(struct nvm_addr addr, uint32_t nlba,
				       const void *buf, uint32_t err)
{
	nvm_test_write_oneshot_err(&addr, nlba, buf, NVM_CMD_SCALAR, err);
}

void nvm_test_vector_write_oneshot_err(struct nvm_addr slba, uint32_t nlba,
				       const void *buf, uint32_t err)
{
	struct nvm_addr addrs[nlba];
	nvm_addr_fill_crange(addrs, slba, nlba);
	nvm_test_write_oneshot_err(addrs, nlba, buf, NVM_CMD_VECTOR, err);
}

void nvm_test_scalar_write_ok(struct nvm_addr addr, uint32_t nlba,
			      const void *buf)
{
	const uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_scalar_write_oneshot_ok(addr, WS_MIN,
						 ubuf + i * SECTOR_SIZE);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_vector_write_ok(struct nvm_addr addr, uint32_t nlba,
			      const void *buf)
{
	const uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_vector_write_oneshot_ok(addr, WS_MIN,
						 ubuf + i * SECTOR_SIZE);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_scalar_read_ok(struct nvm_addr addr, uint32_t nlba, void *buf,
			     const void *expected)
{
	uint8_t *ubuf = buf;
	const uint8_t *uexp = expected;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_scalar_read_oneshot_ok(addr, WS_MIN,
						ubuf + i * SECTOR_SIZE,
						uexp + i * SECTOR_SIZE);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_vector_read_ok(struct nvm_addr addr, uint32_t nlba, void *buf,
			     const void *expected)
{
	uint8_t *ubuf = buf;
	const uint8_t *uexp = expected;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_vector_read_oneshot_ok(addr, WS_MIN,
						ubuf + i * SECTOR_SIZE,
						uexp + i * SECTOR_SIZE);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_scalar_read_ok_predef(struct nvm_addr addr, uint32_t nlba,
				    void *buf)
{
	uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_scalar_read_oneshot_ok_predef(addr, WS_MIN,
						       ubuf + i * SECTOR_SIZE);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_vector_read_ok_predef(struct nvm_addr addr, uint32_t nlba,
				    void *buf)
{
	uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_vector_read_oneshot_ok_predef(addr, WS_MIN,
						       ubuf + i * SECTOR_SIZE);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_scalar_read_err(struct nvm_addr addr, uint32_t nlba, void *buf,
			      uint16_t err)
{
	uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_scalar_read_oneshot_err(addr, WS_MIN,
						 ubuf + i * SECTOR_SIZE, err);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_vector_read_err(struct nvm_addr addr, uint32_t nlba, void *buf,
			      uint16_t err)
{
	uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_vector_read_oneshot_err(addr, WS_MIN,
						 ubuf + i * SECTOR_SIZE, err);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_scalar_write_err(struct nvm_addr addr, uint32_t nlba,
			       const void *buf, uint16_t err)
{
	const uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_scalar_write_oneshot_err(addr, WS_MIN,
						  ubuf + i * SECTOR_SIZE, err);
		addr.l.sectr += WS_MIN;
	}
}

void nvm_test_vector_write_err(struct nvm_addr addr, uint32_t nlba,
			       const void *buf, uint16_t err)
{
	const uint8_t *ubuf = buf;

	for (size_t i = 0; i < nlba; i += WS_MIN) {
		nvm_test_vector_write_oneshot_err(addr, WS_MIN,
						  ubuf + i * SECTOR_SIZE, err);
		addr.l.sectr += WS_MIN;
	}
}

struct nvm_spec_rprt *nvm_test_rprt(struct nvm_addr addr)
{
	struct nvm_addr punit = {
		.l.pugrp = addr.l.pugrp,
		.l.punit = addr.l.punit,
	};

	struct nvm_spec_rprt *rprt = nvm_cmd_rprt(DEV, &punit, 0x0, NULL);
	CU_ASSERT_PTR_NOT_NULL(rprt);

	return rprt;
}

void nvm_test_rprt_assert_wp(struct nvm_addr addr, uint32_t wp)
{
	struct nvm_spec_rprt *rprt = nvm_test_rprt(addr);
	CU_ASSERT(rprt->descr[addr.l.chunk].wp == wp);
	nvm_buf_free(DEV, rprt);
}

void nvm_test_rprt_assert_state(struct nvm_addr addr,
				enum nvm_spec_chunk_state state)
{
	struct nvm_spec_rprt *rprt = nvm_test_rprt(addr);
	CU_ASSERT(rprt->descr[addr.l.chunk].cs == state);
	nvm_buf_free(DEV, rprt);
}

void nvm_test_reset_ok(struct nvm_addr *addr, enum nvm_cmd_opts cmd_opts)
{
	struct nvm_ret ret = { 0 };
	int rc = nvm_cmd_erase(DEV, addr, 1, NULL, cmd_opts, &ret);
	CU_ASSERT(rc == 0);
}

void nvm_test_scalar_reset_ok(struct nvm_addr *addr)
{
	nvm_test_reset_ok(addr, NVM_CMD_SCALAR);
}

void nvm_test_vector_reset_ok(struct nvm_addr *addr)
{
	nvm_test_reset_ok(addr, NVM_CMD_VECTOR);
}

void nvm_test_reset_err(struct nvm_addr *addrs, enum nvm_cmd_opts cmd_opts,
			uint32_t err)
{
	struct nvm_ret ret = { 0 };
	int rc = nvm_cmd_erase(DEV, addrs, 1, NULL, cmd_opts, &ret);

	nvm_test_verify_err(rc, &ret, err);
}

void nvm_test_scalar_reset_err(struct nvm_addr *addrs, uint32_t err)
{
	nvm_test_reset_err(addrs, NVM_CMD_SCALAR, err);
}

void nvm_test_vector_reset_err(struct nvm_addr *addrs, uint32_t err)
{
	nvm_test_reset_err(addrs, NVM_CMD_VECTOR, err);
}
