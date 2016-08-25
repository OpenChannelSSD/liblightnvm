/*
 * nvm - LightNVM get/put block API
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
#ifndef __NVM_H
#define __NVM_H

#include <uthash.h>
#include <nvm_atomic.h>
#include <dflash_ioctl.h>

struct nvm_dev_info {

	char dev_name[NVM_DISK_NAME_LEN];
	uint8_t version;
	uint8_t vendor_opcode;
	uint32_t capabilities;
	uint32_t device_mode;
	char media_manager[NVM_TTYPE_NAME_MAX];

	uint8_t ch_offset;
	uint8_t ch_len;
	uint8_t lun_offset;
	uint8_t lun_len;
	uint8_t pln_offset;
	uint8_t pln_len;
	uint8_t blk_offset;
	uint8_t blk_len;
	uint8_t pg_offset;
	uint8_t pg_len;
	uint8_t sect_offset;
	uint8_t sect_len;

	int fpg_size;
	int sec_size;
	int sec_per_pg;                 /* Number of sectors per flash page */

	uint8_t media_type;
	uint8_t flash_media_type;
	uint8_t num_channels;	        /* Number of channels in device */
	uint8_t num_luns;	        /* Number of LUNs in device */
	uint8_t num_planes;	        /* Number of planes in device */
				        /* aka plane mode: single/dual/quad */
	uint16_t num_blocks;
	uint16_t num_pages;
	uint16_t page_size;
	uint16_t hw_sector_size;	/* Device sector size */
	uint16_t oob_sector_size;	/* Sector out-of-bound area size */

	uint32_t read_typ;
	uint32_t read_max;
	uint32_t prog_typ;
	uint32_t prog_max;
	uint32_t erase_typ;
	uint32_t erase_max;

	uint32_t multiplane_modes;
	uint32_t media_capabilities;
	uint16_t channel_parallelism;

	uint32_t max_phys_sect;         /* Maximum number of sectors per I/O */
};

struct nvm_fpage {
	uint32_t sec_size;
	uint32_t page_size;
	uint32_t pln_pg_size;
	uint32_t max_sec_io;
};

struct nvm_dev {
	struct nvm_dev_info info;	/* Device information */
	struct nvm_fpage fpage;		/* Calculated device flash page sizes */
	atomic_cnt ref_cnt;		/* Reference counter */
	UT_hash_handle hh;		/* Handle to manage open devices */
};

struct nvm_tgt_info {
	int version[3];
	char dev_name[NVM_DISK_NAME_LEN];
	char tgt_name[NVM_DISK_NAME_LEN];
	char tgt_type_name[NVM_TTYPE_NAME_MAX];
};

struct nvm_tgt {
	int fd;				/* Target file descriptor */
	struct nvm_tgt_info info;	/* Target information */
	struct nvm_dev *dev;		/* Associated device */
	atomic_cnt ref_cnt;		/* Reference counter */
	UT_hash_handle hh;		/* Handle to manage to open targets */
};

struct nvm_vblock {
	uint64_t ppa;
	uint16_t flags;
	struct nvm_tgt *tgt;
};

#endif /* __NVM_H */
