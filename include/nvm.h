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

#define NVM_MAGIC_OPCODE_ERASE 0x90     // NVM_OP_ERASE
#define NVM_MAGIC_OPCODE_WRITE 0x91     // NVM_OP_PWRITE
#define NVM_MAGIC_OPCODE_READ 0x92      // NVM_OP_PREAD

struct nvm_dev {
	char name[NVM_DEV_NAME_LEN];	///< Device name e.g. "nvme0n1"
	char path[NVM_DEV_PATH_LEN];	///< Device path e.g. "/dev/nvme0n1"
	struct nvm_addr_fmt fmt;	///< Device address format
	struct nvm_geo geo;		///< Device geometry
	int fd;				///< Device fd / IOCTL handle
};

struct nvm_vblk {
	struct nvm_dev *dev;
	struct nvm_addr addr;
	size_t pos_write;
	size_t pos_read;
};

struct nvm_sblk {
	struct nvm_dev *dev;
	struct nvm_addr bgn;
	struct nvm_addr end;
	struct nvm_geo geo;
	size_t pos_write;
	size_t pos_read;
};

#endif /* __NVM_H */
