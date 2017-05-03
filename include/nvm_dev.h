/*
 * nvm_dev - internal header for liblightnvm
 *
 * Copyright (C) 2015-2017 Javier Gonzáles <javier@cnexlabs.com>
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
#ifndef __INTERNAL_NVM_DEV_H
#define __INTERNAL_NVM_DEV_H

#include <liblightnvm.h>

struct nvm_dev {
	int fd;				///< Device IOCTL handle
	char name[NVM_DEV_NAME_LEN];	///< Device name e.g. "nvme0n1"
	char path[NVM_DEV_PATH_LEN];	///< Device path e.g. "/dev/nvme0n1"
	int nsid;			///< NVME namespace identifier
	uint8_t verid;			///< Open-Channel SSD version identifier
	struct nvm_spec_ppaf_nand ppaf;	///< Device address format
	struct nvm_spec_ppaf_nand_mask mask;///< Device address format mask
	struct nvm_geo geo;		///< Device geometry
	uint64_t ssw;			///< Bit-width for LBA fmt conversion
	uint32_t mccap;			///< Media-controller capabilities
	int pmode;			///< Default plane-mode I/O
	int erase_naddrs_max;		///< Maximum # of cmd-addrs. for erase
	int read_naddrs_max;		///< Maximum # of cmd-addrs. for read
	int write_naddrs_max;		///< Maximum # of cmd-addrs. for write
	int bbts_cached;		///< Whether to cache bbts
	size_t nbbts;			///< Number of entries in cache
	struct nvm_bbt **bbts;		///< Cache of bad-block-tables
	enum nvm_meta_mode meta_mode;	///< Flag to indicate the how meta is w
	struct nvm_be *be;		///< Backend interface
};

#endif /* __INTERNAL_NVM_DEV_H */
