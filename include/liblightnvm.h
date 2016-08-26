/*
 * liblightnvm - Linux Open-Channel I/O interface
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
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

#ifndef DISK_NAME_LEN
#define DISK_NAME_LEN 32
#endif
#ifndef NVM_DISK_NAME_LEN
#define NVM_DISK_NAME_LEN 32
#endif
#ifndef NVM_TTYPE_NAME_MAX
#define NVM_TTYPE_NAME_MAX 48
#endif
#ifndef NVM_TTYPE_MAX
#define NVM_TTYPE_MAX 63
#endif

#ifndef NVM_TGT_NAME_MAX
#define NVM_TGT_NAME_MAX (DISK_NAME_LEN + 5)	/* 5 = strlen(/dev/) */
#endif

/* BITS ALLOCATED FOR THE GENERAL ADDRESS FORMAT */
#define NVM_BLK_BITS (16)
#define NVM_PG_BITS  (16)
#define NVM_SEC_BITS (8)
#define NVM_PL_BITS  (8)
#define NVM_LUN_BITS (8)
#define NVM_CH_BITS  (7)

struct NVM_ADDR {
	union {
		/* General address format */
		struct {
			uint64_t blk		: NVM_BLK_BITS;
			uint64_t pg		: NVM_PG_BITS;
			uint64_t sec		: NVM_SEC_BITS;
			uint64_t pl		: NVM_PL_BITS;
			uint64_t lun		: NVM_LUN_BITS;
			uint64_t ch		: NVM_CH_BITS;
			uint64_t reserved	: 1;
		} g;

		uint64_t ppa;
	};
};

typedef struct nvm_vblock *NVM_VBLOCK;
typedef struct nvm_dev_info *NVM_DEV_INFO;
typedef struct nvm_dev *NVM_DEV;
typedef struct nvm_tgt_info *NVM_TGT_INFO;
typedef struct nvm_tgt *NVM_TGT;

/* TODO: Deprecate this... users should not allocate NVM_DEV and NVM_TGT structures */
NVM_DEV nvm_dev_new(void);
void nvm_dev_free(NVM_DEV *dev);

NVM_DEV nvm_dev_open(const char *dev_name);
void nvm_dev_close(NVM_DEV dev);
void nvm_dev_pr(NVM_DEV dev);

NVM_DEV_INFO nvm_dev_get_info(NVM_DEV dev);
void nvm_dev_info_pr(NVM_DEV_INFO info);

NVM_DEV_INFO nvm_dev_info_new(void);
void nvm_dev_info_free(NVM_DEV_INFO *info);
int nvm_dev_info_fill(NVM_DEV_INFO info, const char *dev_name);

/**
 * @return Number of channels on given device
 */
int nvm_dev_get_nchannels(NVM_DEV);

/**
 * @return Number of NAND dies / luns on given device
 */
int nvm_dev_get_nluns(NVM_DEV);

/**
 * @return Number of planes in a lun on given device
 */
int nvm_dev_get_nplanes(NVM_DEV);

/**
 * @return Number of blocks per plane on given device

 */
int nvm_dev_get_nblocks(NVM_DEV);

/**
 * @return Number of pages per block on given device

 */
int nvm_dev_get_npages(NVM_DEV);

/**
 * @return Number of sectors per page on given device

 */
int nvm_dev_get_nsectors(NVM_DEV);

/**
 * @return Number of bytes per sector on given device

 */
int nvm_dev_get_nbytes(NVM_DEV);



int nvm_dev_get_pln_pg_size(NVM_DEV);
int nvm_dev_get_sec_size(NVM_DEV);

/**
 * Open a target file descriptor for the target named tgt.
 *
 * Descriptor is passed to the nvm_vblock_get / nvm_vblock_put interface.
 * Descriptor is passed to the nvm_vblock_read / nvm_vblock_write interface.
 *
 * By using nvm_tgt_open instead of directly opening a file descriptor to the
 * target path, liblightnvm can keep track of which targets are being used and
 * manage memory accordingly.
 *
 * Returns: A file descriptor to the target named tgt. On error, -1 is returned,
 * in which case errno is set to indicate the error.
 */
NVM_TGT nvm_tgt_open(const char *tgt_name, int flags);

/**
 * Close a target file descriptor.
 */
void nvm_tgt_close(NVM_TGT tgt);
int nvm_tgt_get_fd(NVM_TGT tgt);

NVM_TGT_INFO nvm_tgt_info_new(void);
void nvm_tgt_info_free(NVM_TGT_INFO *info);
int nvm_tgt_info_fill(NVM_TGT_INFO info, const char *tgt_name);
void nvm_tgt_info_pr(NVM_TGT_INFO info);

NVM_VBLOCK nvm_vblock_new(void);
void nvm_vblock_free(NVM_VBLOCK *vblock);
void nvm_vblock_pr(NVM_VBLOCK vblock);

uint64_t nvm_vblock_get_ppa(NVM_VBLOCK vblock);
uint16_t nvm_vblock_get_flags(NVM_VBLOCK vblock);

/**
 * Get ownership of an arbitrary flash block from via the given target.
 *
 * Returns: On success, a flash block is allocated in LightNVM's media manager
 * and vblock is filled up accordingly. On error, -1 is returned, in which case
 * errno is set to indicate the error.
 */
int nvm_vblock_get(NVM_VBLOCK vblock, NVM_TGT tgt);

/**
 * Reserves a block on given target using a specific lun.
 *
 * @param vblock Block created with nvm_vblock_new
 * @param tgt Handle obtained with nvm_tgt_open
 * @param ch Channel from which to reserve via
 * @param lun Lun from which to reserve via
 * @return -1 on error and *errno* set, zero otherwise.
 */
int nvm_vblock_gets(NVM_VBLOCK vblock, NVM_TGT tgt, uint32_t ch, uint32_t lun);

/**
 * Put flash block(s) represented by vblock back to tgt.
 *
 * This action implies that the owner of the flash block previous to this
 * function call no longer owns the flash block, and therefor can no longer
 * submit I/Os to it, or expect that data on it is persisted. The flash block
 * cannot be reclaimed by the previous owner.
 *
 * @return -1 on error and *errno* set, zero otherwise.
 */
int nvm_vblock_put(NVM_VBLOCK vblock);

/**
 * Read count pages starting at ppa_off from tgt into buf using flags
 *
 * fpage_size is the flash page *read* size, which might be smaller than the
 * flash page write size; some controllers support reading at sector granularity
 * (typically 4KB), instead of reading the whole virtual flash page (across
 * planes).
 *
 * XXX(1): For now, we assume that the device supports reading at a sector
 * granurality; we will take this information from the device in the future.
 */
ssize_t nvm_vblock_read(NVM_VBLOCK vblock, void *buf, size_t count,
			size_t ppa_off, int flags);

/**
 * Write count flash pages of size fpage_size.
 *
 * fpage_size is the flash page write size. That is, the size of a virtual flash
 * pages, i.e., spanning flash planes.
 */
ssize_t nvm_vblock_write(NVM_VBLOCK vblock, const void *buf, size_t count,
			 size_t ppa_off, int flags);

/**
 * Instantiates a target with a given target type on top of an nvme device
 * reserving a range of luns.
 *
 * Equivalent of cli: nvme lnvm create  -d <dev_name>
 *                                      -n <tgt_name>
 *                                      -t <tgt_type_name>
 *                                      -O<lun_begin>:<lun_end>
 *
 * @param dev_name Name of the nvme device e.g. nvme0n1
 * @param tgt_name Name of target e.g. nvm0
 * @param tgt_type_name Name of target e.g. dflash, pblk, rrpc
 * @param lun_begin First lun in range
 * @param lun_end Last lun in range
 * @return Error code ?
 */
int nvm_mgmt_tgt_create(const char *tgt_name, const char *tgt_type_name,
			const char *dev_name, int lun_begin, int lun_end);

/**
 * Removes an instantiated target.
 *
 * Equivalent of cli: nvme lnvm remove -n tgt-name
 *
 * @param tgt_name Name of target instance to remove
 * @return ?
 */
int nvm_mgmt_tgt_remove(const char *tgt_name);

/**
 * Initialize structures for beams
 */
int nvm_beam_init(void);

/*
 * Tear down structures for beams.
 */
void nvm_beam_exit(void);

/**
 *
 * @param tgt_name
 * @param lun
 * @param flags
 * @return
 */
int nvm_beam_create(const char *tgt_name, int lun, int flags);

/**
 *
 * @param beam
 * @param flags
 */
void nvm_beam_destroy(int beam, int flags);

/**
 *
 * @param beam
 * @param buf
 * @param count
 * @return
 */
ssize_t nvm_beam_append(int beam, const void *buf, size_t count);

/**
 *
 * @param beam
 * @param buf
 * @param count
 * @param offset
 * @param flags
 * @return
 */
ssize_t nvm_beam_read(int beam, void *buf, size_t count, off_t offset,
		      int flags);

/**
 *
 * @param beam
 * @param flags
 * @return
 */
int nvm_beam_sync(int beam, int flags);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
