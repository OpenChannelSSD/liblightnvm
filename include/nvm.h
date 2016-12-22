/*
 * nvm - internal header for liblightnvm
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
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
#ifndef __NVM_H
#define __NVM_H

#include <liblightnvm.h>

#define UNIVERSAL_SECT_SH 9

/**
 * NVMe command opcodes as defined by LigthNVM specification 1.2
 */
enum spec12_opcodes {
	S12_OPC_SET_BBT = 0xF1,
	S12_OPC_GET_BBT = 0xF2,
	S12_OPC_ERASE = 0x90,
	S12_OPC_WRITE = 0x91,
	S12_OPC_READ = 0x92
};

/**
 * Representation of widths in LBA <-> Physical sector mapping
 *
 * Think of it as a multi-dimensional array in row-major ordered as:
 *
 * channels[]luns[]blocks[]pages[]planes[]sectors[] = sector_nbytes
 *
 * Contains the stride in bytes for each dimension of the geometry.
 */
typedef struct nvm_lba_map {
	size_t channel_nbytes;	///< Number of bytes in a channel
	size_t lun_nbytes;	///< Number of bytes in a LUN
	size_t plane_nbytes;	///< Number of bytes in plane
	size_t block_nbytes;	///< Number of bytes in a block
	size_t page_nbytes;	///< Number of bytes in a page
	size_t sector_nbytes;	///< Number of bytes in a sector
} NVM_LBA_MAP;

/**
 * Encoding descriptor for address formats
 */
struct nvm_addr_fmt {
	union {
		/**
		 * Address format formed as named fields
		 */
		struct {
			uint8_t ch_ofz;		///< Offset in bits for channel
			uint8_t ch_len;		///< Nr. of bits repr. channel
			uint8_t lun_ofz;	///< Offset in bits for LUN
			uint8_t lun_len;	///< Nr. of bits repr. LUN
			uint8_t pl_ofz;		///< Offset in bits for plane
			uint8_t pl_len;		///< Nr. of bits repr. plane
			uint8_t blk_ofz;	///< Offset in bits for block
			uint8_t blk_len;	///< Nr. of bits repr. block
			uint8_t pg_ofz;		///< Offset in bits for page
			uint8_t pg_len;		///< Nr. of bits repr. page
			uint8_t sec_ofz;	///< Offset in bits for sector
			uint8_t sec_len;	///< Nr. of bits repr. sector
		} n;

		/**
		 * Address format formed as anonymous consecutive fields
		 */
		uint8_t a[12];
	};
};

/**
 * Masks for removing bit information from a formatted address
 *
 * E.g. "addr & mask.n.ch" returns the bits representing the channel,
 * this value can then be shifted to obtain the numerical value.
 */
struct nvm_addr_fmt_mask {
	union {
		/**
		 * Address format masks formed as named fields
		 */
		struct {
			uint64_t ch;	///< Mask for channel bits
			uint64_t lun;	///< Mask for lun bits
			uint64_t pl;	///< Mask for pl bits
			uint64_t blk;	///< Mask for blk bits
			uint64_t pg;	///< Mask for page bits
			uint64_t sec;	///< Mask for sector bits
		} n;

		/**
		 * Address format formed as anonymous consecutive fields
		 */
		uint64_t a[6];
	};
};

struct nvm_dev {
	char name[NVM_DEV_NAME_LEN];	///< Device name e.g. "nvme0n1"
	char path[NVM_DEV_PATH_LEN];	///< Device path e.g. "/dev/nvme0n1"
	struct nvm_addr_fmt fmt;	///< Device address format
	struct nvm_addr_fmt_mask mask;	///< Device address format mask
	struct nvm_geo geo;		///< Device geometry
	uint64_t ssw;			///< Bit-width for LBA fmt conversion
	int pmode;			///< Default plane-mode I/O
	int fd;				///< Device fd / IOCTL handle
};

struct nvm_dblk {
	struct nvm_dev *dev;
	struct nvm_addr addr;
	size_t pos_write;
	size_t pos_read;
};

struct nvm_vblk {
	struct nvm_dev *dev;
	struct nvm_addr bgn;
	struct nvm_addr end;
	struct nvm_geo geo;
	size_t pos_write;
	size_t pos_read;
};

void nvm_lba_map_pr(struct nvm_lba_map* map);

/**
 * Prints a humanly readable representation of the give address format
 *
 * @param fmt The address format to print
 */
void nvm_addr_fmt_pr(struct nvm_addr_fmt* fmt);

/**
 * Prints a humanly readable representation of the give address format mask
 *
 * @param fmt The address format mask to print
 */
void nvm_addr_fmt_mask_pr(struct nvm_addr_fmt_mask* fmt);

#endif /* __NVM_H */
