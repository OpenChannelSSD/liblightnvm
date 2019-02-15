/*
 * test_util.h - various utility functions for use when writing tests.
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

#ifndef __TEST_UTIL_H
#define __TEST_UTIL_H

#include <stddef.h>
#include <stdint.h>

#include <liblightnvm.h>
#include <liblightnvm_util.h>

#define NVME_DLFEAT_VAL        0x00

#define NVME_ERR_INVALID_FIELD 0x2
#define NVME_ERR_WRITE_FAULT   0x280
#define NVME_ERR_DULB          0x287

#define LNVM_ERR_OFFLINE_CHUNK 0x2c0
#define LNVM_ERR_INVALID_RESET 0x2c1
#define LNVM_ERR_OUT_OF_ORDER  0x2f2

extern struct nvm_dev *DEV;
extern const struct nvm_geo *GEO;
extern const struct nvm_nvme_ns *NS;

extern uint32_t WS_MIN;
extern uint32_t WS_OPT;
extern uint32_t MW_CUNITS;
extern uint32_t NSECTR;
extern uint32_t SECTOR_SIZE;
extern uint32_t OOB_SIZE;
extern uint32_t MAX_SCALAR_LBAS;

extern int RMODE;

#define SPEC_20_ONLY                                                          \
	if (nvm_dev_get_verid(DEV) != NVM_SPEC_VERID_20) {                    \
		CU_PASS("device is not OCSSD 2.0; skipping test");            \
		return;                                                       \
	}

#define SPEC_12_ONLY                                                          \
	if (nvm_dev_get_verid(DEV) != NVM_SPEC_VERID_12) {                    \
		CU_PASS("device is not OCSSD 1.2; skipping test");            \
		return;                                                       \
	}


/*
 * A function that verifies a request.
 */
typedef void (*nvm_test_verify_fn)(int rc, const struct nvm_ret *ret,
	const char *expected, const char *buf, size_t count, uint16_t err);

/*
 * A function that reads `nlba`s starting at `addr` and asserts that the data
 * read is equal to `expected`.
 */
typedef void (*nvm_test_read_ok_fn)(struct nvm_addr addr, uint32_t nlba,
	char *buf, const char *expected);

/*
 * A function that reads `nlba`s starting at `addr` and asserts that the data
 * read is predefined.
 */
typedef void (*nvm_test_read_ok_predef_fn)(struct nvm_addr addr, uint32_t nlba,
	char *buf);

/*
 * A function that writes `nlba`s starting at `addr` and asserts that the
 * command completes successfully.
 */
typedef void (*nvm_test_write_ok_fn)(struct nvm_addr addr, uint32_t nlba,
	const char *buf);

/*
 * A function that attempts to writes `nlba`s starting at `addr` and asserts
 * that the error in `err` is returned.
 */
typedef void (*nvm_test_write_err_fn)(struct nvm_addr addr, uint32_t nlba,
	const char *buf, uint16_t err);

/*
 * A function that attempts to read `nlba`s starting at `addr` and asserts
 * that the error in `err` is returned.
 */
typedef void (*nvm_test_read_err_fn)(struct nvm_addr addr, uint32_t nlba,
	char *buf, uint16_t err);

/*
 * A function that resets the chunks indicated by `addrs` and asserts that it
 * completed successfully.
 */
typedef void (*nvm_test_reset_ok_fn)(struct nvm_addr *addrs);

/*
 * A function that attemtps to reset the chunks indicated by `addrs` and assert
 * that it failed with `err`.
 */
typedef void (*nvm_test_reset_err_fn)(struct nvm_addr *addrs, uint32_t err);



/*
 * Toggle the DULBE feature.
 */
void nvm_test_set_dulbe(int enable);



/*
 * Assert that the contents of `buf` is predefined data and that the command
 * completed successfully.
 */
void nvm_test_verify_read_ok_predef(int rc, const struct nvm_ret *ret,
	const char *buf, const char *expected, size_t count, uint16_t err);

/*
 * Assert that the contents of `buf` is equal to those in `expected` and that
 * the command completed successfully.
 */
void nvm_test_verify_read_ok(int rc, const struct nvm_ret *ret,
	const char *buf, const char *expected, size_t count, uint16_t err);

/*
 * Asserts that the command was unsuccessful and returned `err`.
 */
void nvm_test_verify_read_err(int rc, const struct nvm_ret *ret,
	const char *buf, const char *expected, size_t count, uint16_t err);


/*
 * Read one sector at `addr` and verify the result using the provided `verify`
 * function.
 *
 * `dulbe controls wether the Deallocated and Unwritten Logical Block Error
 * feature is enabled.
 */
void nvm_test_read_and_verify(struct nvm_addr *addr, int dulbe,
	enum nvm_cmd_opts cmd_opts, nvm_test_verify_fn verify, uint16_t err);

/*
 * Read one sector at `addr` using a scalar command and verify the result using
 * the provided `verify` function.
 *
 * `dulbe controls wether the Deallocated and Unwritten Logical Block Error
 * feature is enabled.
 */
void nvm_test_scalar_read_and_verify(struct nvm_addr *addr, int dulbe,
	nvm_test_verify_fn verify, uint16_t err);

/*
 * Read one sector at `addr` using a vector command and verify the result using
 * the provided `verify` function.
 *
 * `dulbe controls wether the Deallocated and Unwritten Logical Block Error
 * feature is enabled.
 */
void nvm_test_vector_read_and_verify(struct nvm_addr *addr, int dulbe,
	nvm_test_verify_fn verify, uint16_t err);


/*
 * Read `nlba` LBAs using a single command, asserting success.
 */
void nvm_test_read_oneshot_ok(struct nvm_addr *addr, uint32_t nlba, char *buf,
	const char *expected, enum nvm_cmd_opts cmd_opts);

/*
 * Read `nlba` LBAs using a single scalar command, asserting success.
 */
void nvm_test_scalar_read_oneshot_ok(struct nvm_addr addr, uint32_t nlba,
	char *buf, const char *expected);

/*
 * Read `nlba` LBAs using a single vector read command, asserting success.
 */
void nvm_test_vector_read_oneshot_ok(struct nvm_addr slba, uint32_t nlba,
	char *buf, const char *expected);



/*
 * Write `nlba` LBAs using a single command, asserting success.
 */
void nvm_test_write_oneshot_ok(struct nvm_addr *addr, uint32_t nlba,
	const char *buf, enum nvm_cmd_opts cmd_opts);

/*
 * Write `nlba` LBAs using a single scalar command, asserting success.
 */
void nvm_test_scalar_write_oneshot_ok(struct nvm_addr addr, uint32_t nlba,
	const char *buf);

/*
 * Write `nlba` LBAs using a single vector command, asserting success.
 */
void nvm_test_vector_write_oneshot_ok(struct nvm_addr slba, uint32_t nlba,
	const char *buf);



/*
 * Read `nlba` LBAs using a single command, asserting success and that
 * predefined data is returned.
 */
void nvm_test_read_oneshot_ok_predef(struct nvm_addr *addr, uint32_t nlba,
	char *buf, enum nvm_cmd_opts cmd_opts);

/*
 * Read `nlba` LBAs using a single scalar command, asserting success and that
 * predefined data is returned.
 */
void nvm_test_scalar_read_oneshot_ok_predef(struct nvm_addr addr,
	uint32_t nlba, char *buf);

/*
 * Read `nlba` LBAs using a single vector command, asserting success and that
 * predefined data is returned.
 */
void nvm_test_vector_read_oneshot_ok_predef(struct nvm_addr slba,
	uint32_t nlba, char *buf);



/*
 * Read `nlba` LBAs using a single command, asserting that an error equal to
 * `err` is returned.
 */
void nvm_test_read_oneshot_err(struct nvm_addr *addr, uint32_t nlba, char *buf,
	enum nvm_cmd_opts cmd_opts, uint16_t err);

/*
 * Read `nlba` LBAs using a single scalar command, asserting that an error
 * equal to `err` is returned.
 */
void nvm_test_scalar_read_oneshot_err(struct nvm_addr addr, uint32_t nlba,
	char *buf, uint16_t err);

/*
 * Read `nlba` LBAs using a single vector command, asserting that an error
 * equal to `err` is returned.
 */
void nvm_test_vector_read_oneshot_err(struct nvm_addr slba, uint32_t nlba,
	char *buf, uint16_t err);



/*
 * Write `nlba` LBAs using a single command, asserting that an error equal to
 * `err` is returned.
 */
void nvm_test_write_oneshot_err(struct nvm_addr *addr, uint32_t nlba,
	const char *buf, enum nvm_cmd_opts cmd_opts, uint32_t err);

/*
 * Write `nlba` LBAs using a single scalar command, asserting that an error
 * equal to `err` is returned.
 */
void nvm_test_scalar_write_oneshot_err(struct nvm_addr addr, uint32_t nlba,
	const char *buf, uint32_t err);

/*
 * Write `nlba` LBAs using a single vector command, asserting that an error
 * equal to `err` is returned.
 */
void nvm_test_vector_write_oneshot_err(struct nvm_addr slba, uint32_t nlba,
	const char *buf, uint32_t err);



/*
 * Write `nlba`s by splitting into multiple scalar commands of WS_MIN LBAs
 * each, asserting success.
 */
void nvm_test_scalar_write_ok(struct nvm_addr addr, uint32_t nlba,
	const char *buf);

/*
 * Write `nlba`s by splitting into multiple vector commands of WS_MIN LBAs
 * each, asserting success.
 */
void nvm_test_vector_write_ok(struct nvm_addr addr, uint32_t nlba,
	const char *buf);



/*
 * Read `nlba`s by splitting into multiple scalar commands of WS_MIN LBAs
 * each, asserting success.
 */
void nvm_test_scalar_read_ok(struct nvm_addr addr, uint32_t nlba, char *buf,
	const char *expected);

/*
 * Read `nlba`s by splitting into multiple vector commands of WS_MIN LBAs
 * each, asserting success.
 */
void nvm_test_vector_read_ok(struct nvm_addr addr, uint32_t nlba, char *buf,
	const char *expected);



/*
 * Read `nlba`s by splitting into multiple scalar commands of WS_MIN LBAs
 * each, asserting success and that predefined data is returned.
 */
void nvm_test_scalar_read_ok_predef(struct nvm_addr addr, uint32_t nlba,
	char *buf);

/*
 * Read `nlba`s by splitting into multiple vector commands of WS_MIN LBAs
 * each, asserting success and that predefined data is returned.
 */
void nvm_test_vector_read_ok_predef(struct nvm_addr addr, uint32_t nlba,
	char *buf);



/*
 * Read `nlba`s by splitting into multiple scalar commands of WS_MIN LBAs
 * each, asserting that an error equal to `err` is returned.
 */
void nvm_test_scalar_read_err(struct nvm_addr addr, uint32_t nlba, char *buf,
	uint16_t err);

/*
 * Read `nlba`s by splitting into multiple vector commands of WS_MIN LBAs
 * each, asserting that an error equal to `err` is returned.
 */
void nvm_test_vector_read_err(struct nvm_addr addr, uint32_t nlba, char *buf,
	uint16_t err);



/*
 * Read `nlba`s by splitting into multiple scalar commands of WS_MIN LBAs
 * each, asserting that an error equal to `err` is returned.
 */
void nvm_test_scalar_write_err(struct nvm_addr addr, uint32_t nlba,
	const char *buf, uint16_t err);

/*
 * Read `nlba`s by splitting into multiple vector commands of WS_MIN LBAs
 * each, asserting that an error equal to `err` is returned.
 */
void nvm_test_vector_write_err(struct nvm_addr addr, uint32_t nlba,
	const char *buf, uint16_t err);



/*
 * Issue a Get Log Page command and return the report page.
 *
 * @note The returned `struct nvm_spec_rprt` must be deallocated by the user
 * using `nvm_buf_free`.
 */
struct nvm_spec_rprt *nvm_test_rprt(struct nvm_addr addr);

/*
 * Assert that the write pointer of the chunk identified by `addr` is equal to
 *`wp`
 */
void nvm_test_rprt_assert_wp(struct nvm_addr addr, uint32_t wp);

/*
 * Assert that the chunk state of the chunk identified by `addr` is equal to
 * `state`.
 */
void nvm_test_rprt_assert_state(struct nvm_addr addr,
	enum nvm_spec_chunk_state state);


/*
 * Reset the chunks indicated by `addrs` and assert that it completed
 * successfully.
 */
void nvm_test_reset_ok(struct nvm_addr *addrs, enum nvm_cmd_opts cmd_opts);

/*
 * Reset the chunks indicated by `addrs` using a NVMe DSM command and assert
 * that it completed successfully.
 */
void nvm_test_scalar_reset_ok(struct nvm_addr *addrs);

/*
 * Reset the chunks indicated by `addrs` using an OCSSD 2.0 vector reset
 * command and assert that it completed successfully.
 */
void nvm_test_vector_reset_ok(struct nvm_addr *addrs);


/*
 * Attempt to reset the chunks indicated by `addrs` and assert that it failed
 * with `err`.
 */
void nvm_test_reset_err(struct nvm_addr *addrs, enum nvm_cmd_opts cmd_opts,
	uint32_t err);

/*
 * Attempt to reset the chunks indicated by `addrs` using an NVMe DSM command
 * and assert that it failed with `err`.
 */
void nvm_test_scalar_reset_err(struct nvm_addr *addrs, uint32_t err);

/*
 * Attempt to reset the chunks indicated by `addrs` using an OCSSD 2.0 vector
 * reset command and assert that it failed with `err`.
 */
void nvm_test_vector_reset_err(struct nvm_addr *addrs, uint32_t err);

#endif /* __TEST_UTIL_H */
