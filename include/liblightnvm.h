/*
 * User space I/O library for OpenChannelSSDs -- LightNVM
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias González <javier@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
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
#ifndef __LIBLIGHTNVM_H
#define __LIBLIGHTNVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>

#ifndef DISK_NAME_LEN
#define DISK_NAME_LEN 32
#endif
#ifndef NVM_DISK_NAME_LEN
#define NVM_DISK_NAME_LEN 32
#endif

#define NVM_MAGIC_OPCODE_ERASE 0x90     // NVM_OP_ERASE
#define NVM_MAGIC_OPCODE_WRITE 0x91     // NVM_OP_PWRITE
#define NVM_MAGIC_OPCODE_READ 0x92      // NVM_OP_PREAD

#define NVM_MAGIC_FLAG_DUAL 0x1         // NVM_IO_DUAL_ACCESS
#define NVM_MAGIC_FLAG_QUAD 0x2         // NVM_IO_QUAD_ACCESS
#define NVM_MAGIC_FLAG_DEFAULT NVM_MAGIC_FLAG_DUAL

/* BITS ALLOCATED FOR THE GENERAL ADDRESS FORMAT */
#define NVM_BLK_BITS (16)
#define NVM_PG_BITS  (16)
#define NVM_SEC_BITS (8)
#define NVM_PL_BITS  (8)
#define NVM_LUN_BITS (8)
#define NVM_CH_BITS  (7)

/**
 * Encapsulation of generic nvm addressing
 */
typedef struct nvm_addr {
	union {
		struct {
			uint64_t blk         : NVM_BLK_BITS;
			uint64_t pg          : NVM_PG_BITS;
			uint64_t sec         : NVM_SEC_BITS;
			uint64_t pl          : NVM_PL_BITS;
			uint64_t lun         : NVM_LUN_BITS;
			uint64_t ch          : NVM_CH_BITS;
			uint64_t reserved    : 1;
		} g;

		struct {
			uint64_t line        : 63;
			uint64_t is_cached   : 1;
		} c;

		uint64_t ppa;
	};
} NVM_ADDR;

/**
 * Representation of device geometry
 */
typedef struct nvm_geo {
	size_t nchannels;	/// Number of channels on device
	size_t nluns;		/// Number of luns per channel
	size_t nplanes;		/// Number of planes per lun
	size_t nblocks;		/// Number of blocks per plane
	size_t npages;		/// Number of pages per block
	size_t nsectors;	/// Number of sectors per page
	size_t nbytes;		/// Number of bytes per sector

	size_t tbytes;		/// Total number of bytes on device
	size_t vblk_nbytes;	/// Number of bytes per virtual block
	size_t vpg_nbytes;	/// Number of bytes per virtual page
} NVM_GEO;

/**
 * Handle for nvm devices
 */
typedef struct nvm_dev *NVM_DEV;
typedef struct nvm_vblk *NVM_VBLK;
typedef struct nvm_sblk *NVM_SBLK;

void nvm_geo_pr(NVM_GEO geo);

NVM_DEV nvm_dev_open(const char *dev_name);
void nvm_dev_close(NVM_DEV dev);
void nvm_dev_pr(NVM_DEV dev);

/**
 * Returns of the geometry related device information including derived
 * information such as total number of bytes etc.
 *
 * NOTE: See NVM_GEO for the specifics.
 *
 * @return NVM_GEO of given dev
 */
NVM_GEO nvm_dev_attr_geo(NVM_DEV dev);

void *nvm_buf_alloc(NVM_GEO geo, size_t nbytes);

/**
 * Fills `buf` with chars A-Z
 */
void nvm_buf_fill(char *buf, size_t nbytes);

/**
 * Prints `buf` to stdout
 */
void nvm_buf_pr(char *buf, size_t nbytes);

/**
 *      address interface
 */

/**
 * Mark address by setting flags to one of:
 *
 * 0x0 -- GOOD
 * 0x1 -- BAD
 * 0x2 -- GROWN_BAD
 */
ssize_t nvm_addr_mark(NVM_DEV dev, NVM_ADDR list[], int len, uint16_t flags);

ssize_t nvm_addr_erase(NVM_DEV dev, NVM_ADDR list[], int len, uint16_t flags);

ssize_t nvm_addr_write(NVM_DEV dev, NVM_ADDR list[], int len, const void *buf,
                       uint16_t flags);

ssize_t nvm_addr_read(NVM_DEV dev, NVM_ADDR list[], int len, void *buf,
                      uint16_t flags);



void nvm_addr_pr(NVM_ADDR addr);

/**
 *      virtual block interface
 */

NVM_VBLK nvm_vblk_new(void);
NVM_VBLK nvm_vblk_new_on_dev(NVM_DEV dev, NVM_ADDR addr);
void nvm_vblk_free(NVM_VBLK vblk);
void nvm_vblk_pr(NVM_VBLK vblk);

NVM_ADDR nvm_vblk_attr_addr(NVM_VBLK vblk);

/**
 * Get ownership of an arbitrary flash block from the given device.
 *
 * Returns: On success, a flash block is allocated in LightNVM's media manager
 * and vblk is filled up accordingly. On error, -1 is returned, in which case
 * errno is set to indicate the error.
 */
int nvm_vblk_get(NVM_VBLK vblk, NVM_DEV dev);

/**
 * Reserves a block on given device using a specific lun.
 *
 * @param vblk Block created with nvm_vblk_new
 * @param dev Handle obtained with nvm_dev_open
 * @param ch Channel from which to reserve via
 * @param lun Lun from which to reserve via
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
int nvm_vblk_gets(NVM_VBLK vblk, NVM_DEV dev, uint32_t ch, uint32_t lun);

/**
 * Put flash block(s) represented by vblk back to dev.
 *
 * This action implies that the owner of the flash block previous to this
 * function call no longer owns the flash block, and therefor can no longer
 * submit I/Os to it, or expect that data on it is persisted. The flash block
 * cannot be reclaimed by the previous owner.
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
int nvm_vblk_put(NVM_VBLK vblk);

/**
 * Erase an entire vblk
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_erase(NVM_VBLK vblk);

/**
 * Read 'count' bytes from 'vblk' starting at 'offset' into 'buf'
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_pread(NVM_VBLK vblk, void *buf, size_t count, size_t offset);

/**
 * Write 'count' bytes to 'vblk' starting at 'offset' from 'buf'
 *
 * NOTE: Use this for controlling chunked writing, do NOT use this for
 *       random-access.
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_pwrite(NVM_VBLK vblk, const void *buf, size_t count,
			size_t offset);

/**
 * Write 'count' bytes to 'vblk' starting at 'offset' from 'buf'
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_write(NVM_VBLK vblk, const void *buf, size_t count);

/**
 * Read the entire vblk, storing it into buf
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_read(NVM_VBLK vblk, void *buf, size_t count);


/**
 * Create a new sblk.
 */
NVM_SBLK nvm_sblk_new(NVM_DEV dev, int ch_bgn, int ch_end, int lun_bgn,
                      int lun_end, int blk);

/**
 * Destroy an sblk
 */
void nvm_sblk_free(NVM_SBLK sblk);

/**
 * Erase an sblk
 */
ssize_t nvm_sblk_erase(NVM_SBLK sblk);

/**
 * Write content from buffer to sblk
 */
ssize_t nvm_sblk_write(NVM_SBLK sblk, const void *buf, size_t count);

/**
 * Pad the sblk
 */
ssize_t nvm_sblk_pad(NVM_SBLK sblk);

/**
 * Read content from sblk to buffer
 */
ssize_t nvm_sblk_read(NVM_SBLK sblk, void *buf, size_t count);

/**
 * Positioned read from sblk to buffer
 */
ssize_t nvm_sblk_pread(struct nvm_sblk *sblk, void *buf, size_t count,
		       size_t offset);

NVM_DEV nvm_sblk_attr_dev(NVM_SBLK sblk);
NVM_ADDR nvm_sblk_attr_bgn(NVM_SBLK sblk);
NVM_ADDR nvm_sblk_attr_end(NVM_SBLK sblk);
NVM_GEO nvm_sblk_attr_geo(NVM_SBLK sblk);
void nvm_sblk_pr(NVM_SBLK sblk);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
