/*
 * The Open-Channel SSD User Space Library
 *
 * Copyright (C) 2015-2017 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015-2017 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
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
/**
 * @file liblightnvm.h
 */
#ifndef __LIBLIGHTNVM_H
#define __LIBLIGHTNVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <liblightnvm_util.h>
#include <liblightnvm_spec.h>

#define NVM_NADDR_MAX 64

#define NVM_DEV_NAME_LEN 32
#define NVM_DEV_PATH_LEN (NVM_DEV_NAME_LEN + 5)

#define NVM_FLAG_SCRBL 0x200	///< Scrambler ON/OFF: Context sensitive


/**
 * Enumeration of nvm_cmd backends
 */
enum nvm_be_id {
	NVM_BE_ANY	= 0x0,		///< ANY backend
	NVM_BE_IOCTL	= 0x1,		///< IOCTL backend
	NVM_BE_LBD	= 0x1 << 1,	///< IOCTL + LBD backend
	NVM_BE_SPDK	= 0x1 << 2,	///< SPDK backend
	NVM_BE_NOCD	= 0x1 << 3,	///< NON Open-Channel Device backend
};
#define NVM_BE_ALL (NVM_BE_IOCTL | NVM_BE_LBD | NVM_BE_SPDK | NVM_BE_NOCD)

/**
 * Enumeration of nvm_cmd options
 */
enum nvm_cmd_opts {
	NVM_CMD_SYNC		= 0x1 << 4,
	NVM_CMD_ASYNC		= 0x1 << 5,
	NVM_CMD_SCALAR		= 0x1 << 6,
	NVM_CMD_VECTOR		= 0x1 << 7,
	NVM_CMD_PRP		= 0x1 << 8,
	NVM_CMD_SGL		= 0x1 << 9,
	NVM_CMD_SGL_META	= 0x1 << 10,

	NVM_CMD_PIOC		= 0x1 << 11,
	NVM_CMD_PADC		= 0x1 << 12,
};

#define NVM_CMD_MASK_IOMD (NVM_CMD_SYNC | NVM_CMD_ASYNC)
#define NVM_CMD_MASK_ADDR (NVM_CMD_SCALAR | NVM_CMD_VECTOR)
#define NVM_CMD_MASK_PLOD (NVM_CMD_PRP | NVM_CMD_SGL | NVM_CMD_SGL_META)
#define NVM_CMD_MASK_PASS (NVM_CMD_PIOC | NVM_CMD_PADC)
#define NVM_CMD_MASK (NVM_CMD_MASK_IOMD | NVM_CMD_MASK_ADDR | \
			NVM_CMD_MASK_PLOD | NVM_CMD_MASK_PASS)

#define NVM_CMD_DEF_IOMD NVM_CMD_SYNC
#define NVM_CMD_DEF_ADDR NVM_CMD_VECTOR
#define NVM_CMD_DEF_PLOD NVM_CMD_PRP
#define NVM_CMD_DEF_PASS NVM_CMD_PIOC

/**
 * Plane-mode access for OCSSD 1.2 IO
 *
 * TODO: Fix this CMD_OPTS and FLAGS collide
 */
enum nvm_pmode {
	NVM_FLAG_PMODE_SNGL	= 0x0,		///< Single-plane
	NVM_FLAG_PMODE_DUAL	= 0x1,		///< Dual-plane
	NVM_FLAG_PMODE_QUAD	= 0x1 << 1	///< Quad-plane
};
#define NVM_FLAG_DEFAULT (NVM_FLAG_PMODE_SNGL | NVM_FLAG_SCRBL)

/**
 * Flags for device quirks
 */
enum nvm_quirks {
	NVM_QUIRK_PMODE_ERASE_RUNROLL		= 0x1,
	NVM_QUIRK_NSID_BY_NAMECONV		= 0x1 << 1,
	NVM_QUIRK_OOB_READ_1ST4BYTES_NULL	= 0x1 << 2,
	NVM_QUIRK_OOB_2LRG			= 0x1 << 3,
};

/**
 * Opaque handle for NVM devices
 *
 * @struct nvm_dev
 */
struct nvm_dev;

/**
 * Opaque handle for a Scatter Gather List (SGL).
 *
 * @struct nvm_sgl
 */
struct nvm_sgl;

/**
 * Opaque handle for a Scatter Gather List (SGL) pool. A seperate pool should
 * be used for each asynchronous context.
 *
 * @struct nvm_sgl_pool
 */
struct nvm_sgl_pool;

/**
 * Create an SGL pool.
 *
 * @param Associated device.
 *
 * @return An initialized pool that can be used to amortize the cost o
 *  repeated SGL allocations.
 */
struct nvm_sgl_pool *nvm_sgl_pool_create(struct nvm_dev *dev);

/**
 * Destroy an SGL pool (and all SGLs in the pool).
 *
 * @param SGL Pool to destroy.
 */
void nvm_sgl_pool_destroy(struct nvm_sgl_pool *pool);

/**
 * Create an SGL.
 *
 * @see nvm_sgl_destroy
 * @see nvm_sgl_alloc
 * @see nvm_sgl_free
 * @see nvm_sgl_add
 *
 * @param dev Associated device.
 * @param hint Allocation hint.
 *
 * @return On success, an initialized (empty) SGL is returned. On error, NULL
 *  is returned and `errno` is set to indicate any error.
 */
struct nvm_sgl *nvm_sgl_create(struct nvm_dev *dev, int hint);

/**
 * Allocate an SGL from a pool.
 *
 * @see nvm_sgl_create
 *
 * @param pool Pool to allocate SGL from.
 *
 * @return On success, an initialized (empty) SGL is returned. On error, NULL
 *  is returned and `errno` is set to indicate any error.
 */
struct nvm_sgl *nvm_sgl_alloc(struct nvm_sgl_pool *pool);

/**
 * Destroy an SGL, freeing the memory used.
 *
 * @see nvm_sgl_free
 *
 * @param dev Associated device.
 * @param sgl Pointer to SGL as allocated by `nvm_sgl_alloc` or
 *  `nvm_sgl_create`.
 */
void nvm_sgl_destroy(struct nvm_dev *dev, struct nvm_sgl *sgl);

/**
 * Free an SGL, returning it to the pool.
 *
 * @see nvm_sgl_destroy
 *
 * @param dev Pool to return to
 * @param sgl Pointer to SGL as allocated by `nvm_sgl_alloc` or
 *  `nvm_sgl_create`.
 */
void nvm_sgl_free(struct nvm_sgl_pool *pool, struct nvm_sgl *sgl);

/**
 * Reset an SGL.
 *
 * @param sgl Pointer to SGL as allocated by `nvm_sgl_alloc` or
 *  `nvm_sgl_create`.
 */
void nvm_sgl_reset(struct nvm_sgl *sgl);

/**
 * Add an entry to the SGL
 *
 * @see nvm_sgl_alloc
 * @see nvm_buf_alloc
 *
 * @param dev Associated device
 * @param sgl Pointer to sgl as allocated by `nvm_sgl_alloc`
 * @param buf Pointer to buffer as allocated with `nvm_buf_alloc`
 * @param nbytes Size of the given buffer in bytes
 */
int nvm_sgl_add(struct nvm_dev *dev, struct nvm_sgl *sgl, void *buf, size_t nbytes);

/**
 * Virtual block abstraction
 *
 * Facilitates a libc-like read/write and a system-like `pread`/`pwrite`
 * interface to perform I/O on a virtual block spanning multiple blocks of
 * physical NVM.
 *
 * Consult the `nvm_vblk_alloc`, `nvm_vblk_alloc_line` for the different spans
 *
 * @see nvm_vblk_alloc
 * @see nvm_vblk_alloc_line
 *
 * @struct nvm_vblk
 */
struct nvm_vblk;

/**
 * Enumeration of pseudo meta mode
 * TODO: Fix this, this was an old VBLK-specific pseudo-meta-mode
 */
enum nvm_meta_mode {
	NVM_META_MODE_NONE	= 0x0,
	NVM_META_MODE_ALPHA	= 0x1,
	NVM_META_MODE_CONST	= 0x1 << 1
};

/**
 * Enumeration of device bounds
 */
enum nvm_bounds {
	NVM_BOUNDS_CHANNEL	= 0x1,
	NVM_BOUNDS_LUN		= 0x1 << 1,
	NVM_BOUNDS_PLANE	= 0x1 << 2,
	NVM_BOUNDS_BLOCK	= 0x1 << 3,
	NVM_BOUNDS_PAGE		= 0x1 << 4,
	NVM_BOUNDS_SECTOR	= 0x1 << 5,

	NVM_BOUNDS_PUGRP	= 0x1 << 6,
	NVM_BOUNDS_PUNIT	= 0x1 << 7,
	NVM_BOUNDS_CHUNK	= 0x1 << 8,
	NVM_BOUNDS_SECTR	= 0x1 << 9
};

/**
 * Opaque asynchronous context as returned by 'nvm_async_init'
 *
 * @see nvm_async_init
 * @see nvm_async_term
 *
 * @struct nvm_async_ctx
 */
struct nvm_async_ctx;

/**
 * Forward declaration, see definition further down
 */
struct nvm_ret;

/**
 * Signature of function used with asynchronous callbacks.
 */
typedef void (*nvm_async_cb)(struct nvm_ret *ret, void *opaque);

/**
 * IO ASYNC command context per IO, setup this struct inside nvm_ret per call to
 * the nvm_cmd IO functions and set the CMD option NVM_CMD_ASYNC.
 *
 * @see nvm_async_cb
 * @see nvm_async_init
 */
struct nvm_async_cmd_ctx {
	struct nvm_async_ctx *ctx;	///< from nvm_async_init
	nvm_async_cb cb;		///< User provided callback function
	void *cb_arg;			///< User provided callback arguments
};

/**
 * Allocate an asynchronous context for command submission of the given depth
 * for submission of commands to the given device
 *
 * @param dev Associated device
 * @param depth Maximum iodepth / qdepth, maximum number of outstanding commands
 * of the returned context
 * @param flags TBD
 *
 * @return On success, pointer to async. context is returned. On error, NULL is
 * returned and `errno` set to indicate the error
 */
struct nvm_async_ctx *nvm_async_init(struct nvm_dev *dev, uint32_t depth,
				     uint16_t flags);

/**
 * Get the I/O depth of the context.
 *
 * TODO: Fix calling convention
 *
 * @param ctx Asynchronous context
 *
 * @return On success, depth of the given context is returned. On error, 0
 * is returned e.g. errors are silent
 */
uint32_t nvm_async_get_depth(struct nvm_async_ctx *ctx);

/**
 * Get the number of outstanding I/O.
 *
 * TODO: Fix calling convention
 *
 * @param ctx Asynchronous context
 *
 * @return On success, number of outstanding commands are returned. On error, 0
 * is returned e.g. errors are silent
 */
uint32_t nvm_async_get_outstanding(struct nvm_async_ctx *ctx);

/**
 * Tear down the given ASYNC context
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error
 */
int nvm_async_term(struct nvm_dev *dev, struct nvm_async_ctx *ctx);

/**
 * Process completions from the given ASYNC context
 *
 * Set process 'max' to limit number of completions, 0 means no max.
 *
 * @return On success, number of completions processed, may be 0. On error, -1
 * is returned and `errno` set to indicate the error
 */
int nvm_async_poke(struct nvm_dev *dev, struct nvm_async_ctx *ctx,
		   uint32_t max);

/**
 * Wait for completion of all outstanding commands in the given 'ctx'
 *
 * @return On success, number of completions processed, may be 0, is returned.
 * On error, -1 is returned and `errno` set to indicate the error
 */
int nvm_async_wait(struct nvm_dev *dev, struct nvm_async_ctx *ctx);

/**
 * Encapsulation and representation of lower-level error conditions
 *
 * TODO: Consider how to align this with the NVMe completion-entry
 *
 * @struct nvm_ret
 */
struct nvm_ret {
	union {
		struct {
			uint64_t cs;	///< NVM completion status (CS)
		} vio;			///< Vector I/O result

		uint32_t cdw0;		///< NVMe command specific result
	} result;

	uint16_t status;		///< NVMe command status

	struct nvm_async_cmd_ctx async;	///< ASYNC command context
};

/**
 * Obtain string representation of the given plane-mode
 *
 * @param pmode The plane-mode to obtain string representation of
 *
 * @return On success, string representation of the given plane-mode. On error,
 * "UNKN".
 */
const char *nvm_pmode_str(int pmode);

/**
 * Encapsulation of physical/hierarchical/geometric addressing in generic format
 *
 * Although the user need not worry about device specific address formats the
 * user has to know and respect addressing within device specific geometric
 * boundaries.
 *
 * For that purpose one can use the `struct nvm_geo` of an `struct nvm_dev` to
 * obtain device specific geometries.
 */
struct nvm_addr {
	union {
		/**
		 * Address packing and geometric accessors
		 */
		struct {
			uint64_t sec	: 8;	///< Sector address
			uint64_t pg	: 16;	///< Page address
			uint64_t pl	: 8;	///< Plane address
			uint64_t blk	: 16;	///< Block address
			uint64_t lun	: 8;	///< LUN address
			uint64_t ch	: 8;	///< Channel address
		} g;

		struct {
			uint64_t sectr	: 32;	///< Logical Sector in Chunk
			uint64_t chunk	: 16;	///< Chunk in PU
			uint64_t punit	: 8;	///< Parallel Unit (PU) in PUG
			uint64_t pugrp	: 8;	///< Parallel Unit Group (PUG)
		} l;

		// TODO: Removed deprecated PPA-accessor
		uint64_t ppa;			///< Address as raw value

		uint64_t val;			///< Address as raw value
	};
};

/**
 * Representation of device geometry
 *
 * @see nvm_dev_get_geo
 */
struct nvm_geo {
	union {

		/**
		 * Geometry as represented in OCSSD 2.0
		 */
		struct {
			size_t npugrp;		///< # Parallel Unit Groups
			size_t npunit;		///< # Parallel Units in PUG
			size_t nchunk;		///< # Chunks in PU

			size_t nsectr;		///< # Sectors per CNK
			size_t nbytes;		///< # Bytes per SECTOR
			size_t nbytes_oob;	///< # Bytes per SECTOR in OOB
		} l;

		/**
		 * Geometry as represented in OCSSD 1.2
		 */
		struct {
			size_t nchannels;	///< # of channels on device
			size_t nluns;		///< # of LUNs per channel
			size_t nblocks;		///< # of blocks per plane

			size_t nsectors;	///< # of sectors per page
			size_t sector_nbytes;	///< # of bytes per sector
			size_t meta_nbytes;	///< # of bytes for OOB

			size_t nplanes;		///< # of planes per LUN
			size_t npages;		///< # of pages per block
			size_t page_nbytes;	///< # of bytes per page
		} g;

		// TODO: remove this redundant accessor
		struct {
			size_t nchannels;	///< # of channels on device
			size_t nluns;		///< # of LUNs per channel
			size_t nblocks;		///< # of blocks per plane

			size_t nsectors;	///< # of sectors per page
			size_t sector_nbytes;	///< # of bytes per sector
			size_t meta_nbytes;	///< # of bytes for OOB

			size_t nplanes;		///< # of planes per LUN
			size_t npages;		///< # of pages per block
			size_t page_nbytes;	///< # of bytes per page
		};
	};

	size_t tbytes;				///< Total # bytes in geometry
	int verid;				///< Associated dev verid
};

/**
 * Representation of valid values of bad-block-table states
 */
enum nvm_bbt_state {
	NVM_BBT_FREE	= 0x0,		///< Block is free AKA good
	NVM_BBT_BAD	= 0x1,		///< Block is bad
	NVM_BBT_GBAD	= 0x1 << 1,	///< Block marked as grown bad
	NVM_BBT_DMRK	= 0x1 << 2,	///< Block marked by device side
	NVM_BBT_HMRK	= 0x1 << 3	///< Block marked by host side
};

/**
 * Representation of bad-block-table
 *
 * The bad-block-table describes block-state of a given LUN
 *
 * @see nvm_bbt_get, nvm_bbt_set, nvm_bbt_mark, nvm_bbt_free, and nvm_bbt_pr
 */
struct nvm_bbt {
	struct nvm_dev *dev;	///< Device on which the bbt resides
	struct nvm_addr addr;	///< Address of the LUN described by the bbt
	uint64_t nblks;		///< Total # of blocks in lun
	uint32_t nbad;		///< # of manufacturer marked bad blocks
	uint32_t ngbad;		///< # of grown bad blocks
	uint32_t ndmrk;		///< # of device reserved/marked blocks
	uint32_t nhmrk;		///< # of of host reserved/marked blocks
	uint8_t blks[];		///< Array of block status for each block in LUN
};

/**
 * Pass a NVMe command through to the device with minimal intervention
 *
 * When constructing the command then take note of the following:
 *
 * - The CID is managed at a lower level and probably over-written if assigned
 * - When flags include NVM_CMD_PRP then just pass buffers allocated with
 *   `nvm_buf_alloc`, the construction of PRP-lists, assignment to command and
 *   assignment pdst is managed at lower levels
 * - When flags include NVM_CMD_SGL then both data, and meta, when given must be
 *   setup via the `nvm_sgl` helper functions, pdst, data, and meta fields must
 *   be also be set by you
 *
 *   @return On success, 0 is returned. On error, -1 is return, errno set to
 *   indicate the error and `ret`, when provided, containing command status.
 */
int nvm_cmd_pass(struct nvm_dev *dev, struct nvm_nvme_cmd *cmd, void *data,
		 size_t data_nbytes, void *meta, size_t meta_nbytes,
		 int flags, struct nvm_ret *ret);

/**
 * Execute an OCSSD 1.2 identify / OCSSD 2.0 geometry command
 *
 * @note
 * Caller is responsible for de-allocating the returned structure
 *
 * @return On success, pointer to identify structure is returned. On error, NULL
 * is returned and `errno` set to indicate the error and ret filled with
 * lower-level result codes
 */
struct nvm_spec_idfy *nvm_cmd_idfy(struct nvm_dev *dev, struct nvm_ret *ret);

/**
 * Executes one or multiple OCSSD 2.0 get-log-page for chunk-information
 *
 * @note
 * Caller is responsible for de-allocating the returned structure
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr Pointer to a `struct nvm_addr` containing the address of a chunk
 *             to report about
 * @param opt Reporting options, see `enum nvm_spec_chunk_state`
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 *
 * @return On success, pointer report chunk structure is returned. On error,
 * NULL is returned and `errno` set to indicate the error and ret filled with
 * lower-level result codes
 */
struct nvm_spec_rprt *nvm_cmd_rprt(struct nvm_dev *dev, struct nvm_addr *addr,
				   int opt, struct nvm_ret *ret);

/**
 * Find an arbitrary set of 'naddrs' chunk-addresses on the given 'dev', in the
 * given chunk state 'cs' and store them in the provided 'addrs' array
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_cmd_rprt_arbs(struct nvm_dev *dev, int cs, int naddrs,
		      struct nvm_addr addrs[]);

/**
 * Execute an OCSSD 2.0 Get Feature command
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param id Feature identifier (see NVMe 1.3; Figure 84)
 * @param feat Structure defining feature attributes
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_cmd_gfeat(struct nvm_dev *dev, enum nvm_nvme_feat_id id,
		  union nvm_nvme_feat *feat, struct nvm_ret *ret);

/**
 * Execute an OCSSD 2.0 Set Feature command
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param id Feature identifier (see NVMe 1.3; Figure 84)
 * @param feat Structure defining feature attributes
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_cmd_sfeat(struct nvm_dev *dev, enum nvm_nvme_feat_id id,
		  const union nvm_nvme_feat *feat, struct nvm_ret *ret);

/**
 * Execute an OCSSD 1.2 get bad-block-table command
 *
 * @return On success, pointer to bad block table is returned. On error, NULL is
 * returned and `errno` set to indicate the error and ret filled with
 * lower-level result codes
 */
struct nvm_spec_bbt *nvm_cmd_gbbt(struct nvm_dev *dev, struct nvm_addr addr,
				  struct nvm_ret *ret);

/**
 * Find an arbitrary set of 'naddrs' block-addresses on the given 'dev', in the
 * given block state 'bs' and store them in the provided 'addrs' array
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error
 */
int nvm_cmd_gbbt_arbs(struct nvm_dev *dev, int bs, int naddrs,
		      struct nvm_addr addrs[]);

/**
 * Execute an OCSSD 1.2 set bad block table command
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error.
 */
int nvm_cmd_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs, int naddrs,
		 uint16_t flags, struct nvm_ret *ret);

/**
 * Execute an OCSSD 1.2 erase / OCSSD 2.0 reset command
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *meta, uint16_t flags, struct nvm_ret *ret);

/**
 * Execute an OCSSD 1.2 / 2.0 vector-write command
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  const void *data, const void *meta, uint16_t flags,
		  struct nvm_ret *ret);

/**
 * Execute an OCSSD 1.2 / 2.0 vector-read command
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 void *data, void *meta, uint16_t flags, struct nvm_ret *ret);

/**
 * Execute an OCSSD 2.0 vector-copy command
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_copy(struct nvm_dev *dev, struct nvm_addr src[],
		 struct nvm_addr dst[], int naddrs, uint16_t flags,
		 struct nvm_ret *ret);

/**
 * @return the "major" version of the library
 */
int nvm_ver_major(void);

/**
 * @return the "minor" version of the library
 */
int nvm_ver_minor(void);

/**
 * @return the "patch" version of the library
 */
int nvm_ver_patch(void);

/**
 * Prints version information about the library
 */
void nvm_ver_pr(void);

/**
 * Prints a humanly readable description of given boundary mask
 */
void nvm_bounds_pr(int mask);

/**
 * Prints a humanly readable representation the given `struct nvm_ret`
 *
 * @param ret Pointer to the `struct nvm_ret` to print
 */
void nvm_ret_pr(const struct nvm_ret *ret);

/**
 * Clears/resets the given `struct nvm_ret`.
 *
 * @param ret Pointer to `struct nvm_ret`.
 */
void nvm_ret_clear(struct nvm_ret *ret);

/**
 * Retrieves a bad block table from device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr Address of the LUN to retrieve bad-block-table for
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 *
 * @return On success, a pointer to the bad-block-table is returned. On error,
 * NULL is returned, `errno` set to indicate the error and ret filled with
 * lower-level result codes
 */
const struct nvm_bbt *nvm_bbt_get(struct nvm_dev *dev, struct nvm_addr addr,
				  struct nvm_ret *ret);

/**
 * Updates the bad-block-table on given device using the provided bbt
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param bbt The bbt to write to device
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_set(struct nvm_dev *dev, const struct nvm_bbt *bbt,
		struct nvm_ret *ret);

/**
 * Mark addresses good, bad, or host-bad.
 *
 * @note
 * The addresses given to this function are interpreted as block addresses, in
 * contrast to `nvm_cmd_write`, and `nvm_cmd_read` which interpret addresses and
 * sector addresses.
 *
 * @see `enum nvm_bbt_state`
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addrs Array of memory address
 * @param naddrs Length of memory address array
 * @param flags 0x0 = GOOD, 0x1 = BAD, 0x2 = GROWN_BAD, as well as access mode
 * @param ret Pointer to structure in which to store lower-level status and
 *            result.
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_mark(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 uint16_t flags, struct nvm_ret *ret);

/**
 * Persist the bad-block-table at `addr` on device and deallocate managed memory
 * for the given bad-block-table describing the LUN at `addr`.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr Address of the LUN to flush bad-block-table for
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_flush(struct nvm_dev *dev, struct nvm_addr addr,
		  struct nvm_ret *ret);

/**
 * Persist all bad-block-tables associated with the given `dev`
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_flush_all(struct nvm_dev *dev, struct nvm_ret *ret);

/**
 * Allocate a copy of the given bad-block-table
 *
 * @param bbt Pointer to the bad-block-table to copy
 *
 * @return On success, a pointer to a write-able copy of the given bbt is
 * returned. On error, NULL is returned and `errno` set to indicate the error
 */
struct nvm_bbt *nvm_bbt_alloc_cp(const struct nvm_bbt *bbt);

/**
 * Destroys a given bad-block-table
 *
 * @param bbt The bad-block-table to destroy
 */
void nvm_bbt_free(struct nvm_bbt *bbt);

/**
 * Prints a humanly readable representation of the given bad-block-table
 *
 * @param bbt The bad-block-table to print
 */
void nvm_bbt_pr(const struct nvm_bbt *bbt);

/**
 * Prints a humanly readable representation of the given bad-block-table state
 */
void nvm_bbt_state_pr(int state);

/**
 * Prints human readable representation of the given geometry
 */
void nvm_geo_pr(const struct nvm_geo *geo);

/**
 * Creates a handle to given device path
 *
 * @param dev_path Path of the device to open e.g. "/dev/nvme0n1"
 *
 * @return A handle to the device
 */
struct nvm_dev *nvm_dev_open(const char *dev_path);

/**
 * Creates a handle to given device path
 *
 * @param dev_path Path of the device to open e.g. "/dev/nvme0n1"
 * @param flags Flags for opening device in different modes
 *
 * @return A handle to the device
 */
struct nvm_dev *nvm_dev_openf(const char *dev_path, int flags);

/**
 * Destroys device-handle
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
void nvm_dev_close(struct nvm_dev *dev);

/**
 * Prints misc. device attribute associated with the given handle
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
void nvm_dev_attr_pr(const struct nvm_dev *dev);

/**
 * Prints all information about the device associated with the given handle
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
void nvm_dev_pr(const struct nvm_dev *dev);

/**
 * Returns the file-descriptor associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, file descriptor is returned
 */
int nvm_dev_get_fd(const struct nvm_dev *dev);

/**
 * Returns the name associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, string is returned. On error, NULL is returned.
 */
const char *nvm_dev_get_name(const struct nvm_dev *dev);

/**
 * Returns the path associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, string is returned. On error, NULL is returned.
 */
const char *nvm_dev_get_path(const struct nvm_dev *dev);

/**
 * Returns the NVME namespace identifier of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, NVME namespace identifier is returned.
 */
int nvm_dev_get_nsid(const struct nvm_dev *dev);

/**
 * Returns the OCSSD version identifier of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, verid is returned. On error, -1 is returned and `errno`
 * set to indicate the error
 */
int nvm_dev_get_verid(const struct nvm_dev *dev);

/**
 * Returns the media-controller capabilities mask of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, capabilities mask is returned
 */
uint32_t nvm_dev_get_mccap(const struct nvm_dev *dev);

/**
 * Returns the geometry of the given device
 *
 * @note
 * See `struct nvm_geo` for the specifics of the returned geometry
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return The geometry (struct nvm_geo) of given device handle
 */
const struct nvm_geo *nvm_dev_get_geo(const struct nvm_dev *dev);

/*
 * Returns the namespace of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`.
 *
 * @return On success, pointer to namespace structure. On error, NULL is
 * returned and errno is set to indicate the error.
 */
const struct nvm_nvme_ns *nvm_dev_get_ns(const struct nvm_dev *dev);

/**
 * Returns the minimum write size, in number of sectors, for the given device.
 *
 * @note
 * This is only defined in OCSSD 2.0. For OCSSD 1.2 devices an estimate is
 * returned based on device geometry
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, the minimal write size is returned. On error, -1 is
 * returned `errno` set to indicate the error
 */
int nvm_dev_get_ws_min(const struct nvm_dev *dev);

/**
 * Returns the optimal write size, in number of sectors, for the given device.
 *
 * @note
 * This is only defined in OCSSD 2.0. For OCSSD 1.2 devices an estimate is
 * returned based on device geometry
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, the optimal write size is returned. On error, -1 is
 * returned `errno` set to indicate the error
 */
int nvm_dev_get_ws_opt(const struct nvm_dev *dev);

/**
 * Returns the minimal write-cache units for the given device
 *
 * @note
 * This is only defined in OCSSD 2.0. For OCSSD 1.2 devices an estimate is
 * returned based on device geometry
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, the minimal write cache unites is returned. On error, -1
 * is returned `errno` set to indicate the error
 */
int nvm_dev_get_mw_cunits(const struct nvm_dev *dev);

/**
 * Returns the mask of quirks for the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, quirk mask is returned. On error, -1 is returned and
 * `errno` set to indicate the error
 */
int nvm_dev_get_quirks(const struct nvm_dev *dev);

/**
 * Set the default plane-mode for the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param quirks Mask of `enum nvm_quirks`
 *
 * @return On success, 0 is returned. On error, -1 is returned on error and
 * `errno` set to indicate the error
 */
int nvm_dev_set_quirks(struct nvm_dev *dev, int quirks);


/**
 * Returns the OCSSD 2.0 device format
 *
 * @note
 * Applies only to OCSSD 2.0 devices
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, LBA format is returned
 */
const struct nvm_spec_lbaf *nvm_dev_get_lbaf(const struct nvm_dev *dev);

/**
 * Returns the ppa-format of the given device
 *
 * @note
 * Applies only to OCSSD 1.2 devices
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, a pointer to the PPA-format is returned. On error, NULL
 * is returned and `errno` set to indicate the error
 */
const struct nvm_spec_ppaf_nand *nvm_dev_get_ppaf(const struct nvm_dev *dev);

/**
 * Returns the PPA-format mask of the given device
 *
 * @note
 * Applies only to OCSSD 1.2 devices
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, a pointer to the PPA-format mask is returned. On error,
 * NULL is returned and `errno` set to indicate the error
 */
const struct nvm_spec_ppaf_nand_mask *nvm_dev_get_ppaf_mask(const struct nvm_dev *dev);

/**
 * Returns the default plane_mode of the given device
 *
 * @note
 * Applies only to OCSSD 1.2 devices
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, pmode flag is returned
 */
int nvm_dev_get_pmode(const struct nvm_dev *dev);

/**
 * Set the default plane-mode for the given device
 *
 * @note
 * Applies only to OCSSD 1.2 devices
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param pmode Default plane-mode
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_dev_set_pmode(struct nvm_dev *dev, int pmode);

/**
 * Returns whether caching is enabled for bad-block-tables on the device.
 *
 * @note
 * Applies only to OCSSD 1.2 device
 *
 * @note
 * 0 = cache disabled
 * 1 = cache enabled
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_bbts_cached(const struct nvm_dev *dev);

/**
 * Sets whether retrieval and changes to bad-block-tables should be cached.
 *
 * @note
 * Applies only to OCSSD 1.2 device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param bbts_cached 1 = cache enabled, 0 = cache disabled
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_dev_set_bbts_cached(struct nvm_dev *dev, int bbts_cached);

/**
 * Returns the 'meta-mode' of the given device
 *
 * TODO: Move this... it is a vblk feature
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, meta-mode is returned
 */
int nvm_dev_get_meta_mode(const struct nvm_dev *dev);

/**
 * Set the default 'meta-mode' of the given device
 *
 * The meta-mode is a setting used by the nvm_vblk interface to write
 * pseudo-meta data to the out-of-bound area.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param meta_mode One of: NVM_META_MODE_[NONE|ALPHA|CONST]
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set to
 * indicate the error.
 */
int nvm_dev_set_meta_mode(struct nvm_dev *dev, int meta_mode);

/**
 * Returns the maximum number of addresses used by VBLK in vector-erase commands
 *
 * @note
 * This is a deprecated `nvm_vblk` feature and it will be removed / re-defined
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_erase_naddrs_max(const struct nvm_dev *dev);

/**
 * Set the maximum number of addresses used by VBLK in vector-erase commands
 *
 * @note
 * This is a deprecated `nvm_vblk` feature and it will be removed / re-defined
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param naddrs The maximum
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_dev_set_erase_naddrs_max(struct nvm_dev *dev, int naddrs);

/**
 * Returns the maximum number of addresses used by VBLK in vector-write commands
 *
 * @note
 * This is a deprecated `nvm_vblk` feature and it will be removed / re-defined
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_write_naddrs_max(const struct nvm_dev *dev);

/**
 * Set the maximum number of addresses used by VBLK in vector-write commands
 *
 * @note
 * This is a deprecated `nvm_vblk` feature and it will be removed / re-defined
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param naddrs The maximum
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_dev_set_write_naddrs_max(struct nvm_dev *dev, int naddrs);

/**
 * Returns the maximum number of addresses used by VBLK in vector-read commands
 *
 * @note
 * This is a deprecated `nvm_vblk` feature and it will be removed / re-defined
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_read_naddrs_max(const struct nvm_dev *dev);

/**
 * Set the maximum number of addresses used by VBLK in vector-read commands
 *
 * @note
 * This is a deprecated `nvm_vblk` feature and it will be removed / re-defined
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param naddrs The maximum
 *
 * @return 0 on success, -1 on error and `errno` set to indicate the error.
 */
int nvm_dev_set_read_naddrs_max(struct nvm_dev *dev, int naddrs);

/**
 * Returns the backend identifier associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_be_id(const struct nvm_dev *dev);

/**
 * Allocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required
 * by the backend for IO
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using `nvm_buf_free`
 *
 * @see nvm_buf_free
 *
 * @param dev The device to allocate IO buffers for
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the
 * allocated buffer. If NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error,
 * NULL is returned and `errno` set to indicate the error.
 */
void *nvm_buf_alloc(const struct nvm_dev *dev, size_t nbytes, uint64_t *phys);

/**
 * Reallocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required
 * by the backend for IO
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using `nvm_buf_free`
 *
 * @see nvm_buf_free
 *
 * @param dev The device to allocate IO buffers for
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the
 * allocated buffer. If NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error,
 * NULL is returned and `errno` set to indicate the error.
 */
void *nvm_buf_realloc(const struct nvm_dev *dev, void *buf, size_t nbytes,
		      uint64_t *phys);

/**
 * Free the given IO buffer allocated with `nvm_buf_alloc`
 *
 * @see nvm_buf_alloc
 *
 * @param dev Pointer to the device which the IO buffer was allocated for
 * @param buf Pointer to a buffer allocated with `nvm_buf_alloc`
 */
void nvm_buf_free(const struct nvm_dev *dev, void *buf);

/**
 * Retrieve the physical address of the given buffer
 *
 * @param dev Pointer to the device which the IO buffer was allocated for
 * @param buf Pointer to a buffer allocated with `nvm_buf_alloc`
 * @param phys A pointer to the variable to hold the physical address of the
 * given buffer.
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set to
 * indicate the error.
 */
int nvm_buf_vtophys(const struct nvm_dev *dev, void *buf, uint64_t *phys);

/**
 * Allocate a buffer of virtual memory of the given 'nbytes' and 'alignment'
 *
 * @note
 * You must use `nvm_buf_virt_free` to de-allocate the buffer
 *
 * @param alignment The alignment in bytes
 * @param nbytes The size of the buffer in bytes
 *
 * @return A pointer to the allocated memory. On error: NULL is returned and
 * `errno` set appropriatly
 */
void *nvm_buf_virt_alloc(size_t alignment, size_t nbytes);

/**
 * Free the given virtual memory buffer
 *
 * @param buf Pointer to a buffer allocated with `nvm_buf_virt_alloc`
 */
void nvm_buf_virt_free(void *buf);

/**
 * Fills `buf` with chars A-Z
 *
 * @param buf Pointer to the buffer to fill
 * @param nbytes Amount of bytes to fill in buf
 */
void nvm_buf_fill(char *buf, size_t nbytes);

/**
 * Prints `buf` to stdout
 *
 * @param buf Pointer to the buffer to print
 * @param nbytes Amount of bytes of buf to print
 */
void nvm_buf_pr(const char *buf, size_t nbytes);

/**
 * Returns the number of bytes where expected is different from actual
 */
size_t nvm_buf_diff(const char *expected, const char *actual, size_t nbytes);

/**
 * Prints the number and value of bytes where expected is different from actual
 */
void nvm_buf_diff_pr(const char *expected, const char *actual, size_t nbytes);

/**
 * Write content of buffer into file
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Destination where buffer will be dumped to
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error
 */
int nvm_buf_to_file(char *buf, size_t nbytes, const char *path);

/**
 * Read content of file into buffer
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Source to read from
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error
 */
int nvm_buf_from_file(char *buf, size_t nbytes, const char *path);

/**
 * Encapsulation of a IO buffer-set for common-case setup
 *
 * This structure is allocated and initialized by `nvm_buf_set_alloc`, and
 * de-allocated by `nvm_buf_set_free`
 *
 * @struct nvm_buf_set
 */
struct nvm_buf_set {
	struct nvm_dev *dev;	///< Device for which IO buffers are allocated

	char *write;		///< Data buffer to use for writes
	char *write_meta;	///< Meta / OOB buffer to use for writes

	char *read;		///< Data buffer to use for reads
	char *read_meta;	///< Meta / OOB buffer to use for reads

	size_t nbytes;		///< # of bytes per data buffer
	size_t nbytes_meta;	///< # of bytes per meta buffer
};

/**
 * Prints the given nvm_buf_set
 */
void nvm_buf_set_pr(struct nvm_buf_set *bufs);

struct nvm_buf_set *nvm_buf_set_alloc(struct nvm_dev *dev, size_t nbytes,
				      size_t nbytes_meta);

void nvm_buf_set_fill(struct nvm_buf_set *bufs);

void nvm_buf_set_free(struct nvm_buf_set *bufs);

/**
 * Checks whether the given address exceeds bounds of the geometry of the given
 * device
 *
 * @param addr The addr to check
 * @param dev The device of which to check geometric bounds against
 *
 * @return A mask of exceeded boundaries
 */
int nvm_addr_check(struct nvm_addr addr, const struct nvm_dev *dev);

/**
 * Compute log-page-offset (lpo) in the NVMe chunk-information get-log-page
 *
 * That is, the location of the chunk-descriptor in the log-page, for the chunk
 * at the given address in generic-format
 *
 * @note
 * This is a helper for function for `nvm_cmd_rprt`, as a library user you will
 * most likely not have a use for it
 *
 * @return the log page offset (lpo) for the given addr
 */
uint64_t nvm_addr_gen2lpo(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Inverse function of `nvm_addr_gen2lpo`
 *
 * @note
 * This is a helper for function for `nvm_cmd_rprt`, as a library user you will
 * most likely not have a use for it
 *
 * @return the page offset (lpo) for the given addr
 */
struct nvm_addr nvm_addr_lpo2gen(struct nvm_dev *dev, uint64_t lpo);

/**
 * Converts an address, in generic-format, to device-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The address, in generic-format, to convert
 *
 * @return Address in device-format
 */
uint64_t nvm_addr_gen2dev(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Converts an address, in device-format, to generic-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The address, in device-format, to convert
 *
 * @return Address in generic-format
 */
struct nvm_addr nvm_addr_dev2gen(struct nvm_dev *dev, uint64_t addr);

/**
 * Converts an address, in generic-format, to Linux Block Device offset
 *
 * @note
 * This is a helper for function for the LBD backend, as a library user you will
 * most likely not have a use for it
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The address, in generic-format, to convert
 *
 * @return LBD offset
 */
uint64_t nvm_addr_gen2off(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Converts a Linux Block Device offset to an address in generic-format
 *
 * @note
 * This is a helper for function for the LBD backend, as a library user you will
 * most likely not have a use for it
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param off LBD offset
 *
 * @return Address in generic-format
 */
struct nvm_addr nvm_addr_off2gen(struct nvm_dev *dev, uint64_t off);

/**
 * Converts an address, in device-format, to Linux Block Device offset
 *
 * @note
 * This is a helper for function for the LBD backend, as a library user you will
 * most likely not have a use for it
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on device-format to convert
 *
 * @return Physical address on lba-offset-format
 */
uint64_t nvm_addr_dev2off(struct nvm_dev *dev, uint64_t addr);

/**
 * Converts a Linux Block Device offset to an address in device-format
 *
 * @note
 * This is a helper for function for the LBD backend, as a library user you will
 * most likely not have a use for it
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on device-format to convert
 *
 * @return Physical address on lba-offset-format
 */
uint64_t nvm_addr_off2dev(struct nvm_dev *dev, uint64_t addr);

/*
 * Fills out the `addrs` array defined as the contiguous range
 * `[addr, addr + naddrs[`.
 *
 * @note This is a convenience function intended for use in preparation for
 * calls to `nvm_cmd_{write|read}` when using using the, currently default,
 * command option `NVM_CMD_VECTOR`.
 *
 * @note Address validity is not verified and this function can produce invalid
 * addresses when crossing geometric boundaries.
 *
 * @param addrs The array of addresses to fill
 * @param addr The starting address of address-range
 * @param naddrs The number of addresses in the address-range
 */
void nvm_addr_fill_crange(struct nvm_addr *addrs, struct nvm_addr addr,
			  uint32_t naddrs);

/**
 * Prints a hexidecimal representation of the given address value
 */
void nvm_addr_pr(const struct nvm_addr addr);

/**
 * Prints a humanly readable representation of the given list of addresses
 * according to the geometry of the given device (spec 1.2 or 2.0)
 */
void nvm_addr_prn(const struct nvm_addr *addr, unsigned int naddrs,
		  const struct nvm_dev *dev);

/**
 * Allocate a virtual block, spanning a given set of physical blocks
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addrs Set of block-addresses forming the virtual block
 * @param naddrs The number of addresses in the address-set
 *
 * @return On success, an opaque pointer to the initialized virtual block is
 * returned. On error, NULL and `errno` set to indicate the error.
 */
struct nvm_vblk *nvm_vblk_alloc(struct nvm_dev *dev, struct nvm_addr addrs[],
				int naddrs);

/**
 * Allocate a virtual block (spanning planes, channels, and LUNs)
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param ch_bgn Beginning of the channel span, as inclusive index
 * @param ch_end End of the channel span, as inclusive index
 * @param lun_bgn Beginning of the LUN span, as inclusive index
 * @param lun_end End of the LUN span, as inclusive index
 * @param blk Block index
 *
 * @return On success, an opaque pointer to the initialized virtual block is
 * returned.  On error, NULL and `errno` set to indicate the error.
 */
struct nvm_vblk *nvm_vblk_alloc_line(struct nvm_dev *dev, int ch_bgn,
				     int ch_end, int lun_bgn, int lun_end,
				     int blk);

/**
 * Set the command mode for the virtual block to async.
 */
int nvm_vblk_set_async(struct nvm_vblk *vblk, uint32_t depth);

/**
 * Set the command mode for the virtual block to scalar.
 */
int nvm_vblk_set_scalar(struct nvm_vblk *vblk);

/**
 * Destroy a virtual block
 *
 * @param vblk The virtual block to destroy
 */
void nvm_vblk_free(struct nvm_vblk *vblk);

/**
 * Erase a virtual block
 *
 * @note
 * Erasing a vblk will reset internal position pointers
 *
 * @param vblk The virtual block to erase
 *
 * @return On success, the number of bytes erased is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_vblk_erase(struct nvm_vblk *vblk);

/**
 * Write to a virtual block
 *
 * @note
 * buf must be aligned to device geometry, see struct nvm_geo and nvm_buf_alloc
 * count must be a multiple of min-size, see struct nvm_geo
 * do not mix use of nvm_vblk_pwrite with nvm_vblk_write on the same virtual
 * block
 *
 * @param vblk The virtual block to write to
 * @param buf Write content starting at buf
 * @param count The number of bytes to write
 *
 * @return On success, the number of bytes written is returned and vblk
 * internal position is updated. On error, -1 is returned and `errno` set to
 * indicate the error.
 */
ssize_t nvm_vblk_write(struct nvm_vblk *vblk, const void *buf, size_t count);

/**
 * Write to a virtual block at a given offset
 *
 * @note
 * buf must be aligned to device geometry, see struct nvm_geo and nvm_buf_alloc
 * count must be a multiple of min-size, see struct nvm_geo
 * offset must be a multiple of min-size, see struct nvm_geo
 * do not mix use of nvm_vblk_pwrite with nvm_vblk_write on the same virtual
 * block
 *
 * @param vblk The virtual block to write to
 * @param buf Write content starting at buf
 * @param count The number of bytes to write
 * @param offset Start writing offset bytes within virtual block
 *
 * @return On success, the number of bytes written is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf, size_t count,
			size_t offset);

/**
 * Pad the virtual block with synthetic data
 *
 * @note
 * Assumes that you have used nvm_vblk_write and now want to fill the remaining
 * virtual block in order to meet block write-before-read constraints
 *
 * @param vblk The virtual block to pad
 *
 * @return On success, the number of bytes padded is returned and internal
 * position is updated. On error, -1 is returned and `errno` set to indicate the
 * error.
 */
ssize_t nvm_vblk_pad(struct nvm_vblk *vblk);

/**
 * Read from a virtual block
 */
ssize_t nvm_vblk_read(struct nvm_vblk *vblk, void *buf, size_t count);

/**
 * Read from a virtual block at given offset
 */
ssize_t nvm_vblk_pread(struct nvm_vblk *vblk, void *buf, size_t count,
		       size_t offset);

/**
 * Copy the virtual block 'src' to the virtual block 'dst'
 *
 * @return On success, the number of bytes copied is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_vblk_copy(struct nvm_vblk *src, struct nvm_vblk *dst, int flags);

/**
 * Retrieve the device associated with the given virtual block
 *
 * @param vblk The entity to retrieve information from
 */
struct nvm_dev *nvm_vblk_get_dev(struct nvm_vblk *vblk);

/**
 * Retrieve the set of addresses defining the virtual block
 *
 * @param vblk The entity to retrieve information from
 */
struct nvm_addr *nvm_vblk_get_addrs(struct nvm_vblk *vblk);

/**
 * Retrieve the number of addresses in the address set of the virtual block
 *
 * @param vblk The entity to retrieve information from
 */
int nvm_vblk_get_naddrs(struct nvm_vblk *vblk);

/**
 * Retrieve the size, in bytes, of a given virtual block
 *
 * @param vblk The entity to retrieve information from
 */
size_t nvm_vblk_get_nbytes(struct nvm_vblk *vblk);

/**
 * Retrieve the read cursor position for the given virtual block
 *
 * @param vblk The entity to retrieve information from
 */
size_t nvm_vblk_get_pos_read(struct nvm_vblk *vblk);

/**
 * Retrieve the write cursor position for the given virtual block
 *
 * @param vblk The entity to retrieve information from
 */
size_t nvm_vblk_get_pos_write(struct nvm_vblk *vblk);

/**
 * Set the read cursor position for the given virtual block
 *
 * @param vblk The vblk to change
 * @param pos The new read cursor
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error
 */
int nvm_vblk_set_pos_read(struct nvm_vblk *vblk, size_t pos);

/**
 * Set the write cursor position for the given virtual block
 *
 * @param vblk The vblk to change
 * @param pos The new write cursor
 *
 * @return On success, 0 is returned. On error, -1 is returned and `errno` set
 * to indicate the error
 */
int nvm_vblk_set_pos_write(struct nvm_vblk *vblk, size_t pos);

/**
 * Print the virtual block in a humanly readable form
 *
 * @param vblk The entity to print information about
 */
void nvm_vblk_pr(struct nvm_vblk *vblk);

/**
 * Boilerplate for working with the API
 *
 * Encapsulated in a struct such that example code can focus on the interesting
 * parts and the usual boiler-plate code needed to get things going.
 */
struct nvm_bp {
	struct nvm_dev *dev;
	const struct nvm_geo *geo;
	struct nvm_buf_set *bufs;
	struct nvm_vblk *vblk;
	size_t ws_opt;
	size_t naddrs;
	struct nvm_addr addrs[];
};

void nvm_bp_pr(const struct nvm_bp *bp);

/**
 * Use argv as 'nvm_bp_init(argv[1], argv[2], argv[3])'
 *
 * @return On success, a initialized boiler-plate is returned. On error, NULL is
 * returned and `errno` set to indicate the error
 */
struct nvm_bp *nvm_bp_init_from_args(int argc, char **argv);

/**
 * argv[3]: naddrs
 * argv[2]: Backend Identifier as hex, eg. NVM_BE_IOCTL(0x1), NVM_BE_SPDK(0x4)
 * argv[1]: Device identifier, eg. "/dev/nvme0n1" or "traddrs:0000:00:01.0"
 */
struct nvm_bp *nvm_bp_init(const char *dev_ident, int flags, int naddrs);

void nvm_bp_term(struct nvm_bp *bp);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
