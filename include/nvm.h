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

#include <liblightnvm.h>
#include <uthash.h>
#include <nvm_atomic.h>

#define NVM_MAGIC_OPCODE_ERASE 0x90     // NVM_OP_ERASE
#define NVM_MAGIC_OPCODE_WRITE 0x91     // NVM_OP_PWRITE
#define NVM_MAGIC_OPCODE_READ 0x92      // NVM_OP_PREAD
#define NVM_MAGIC_FLAG_DUAL 0x1         // NVM_IO_DUAL_ACCESS
#define NVM_MAGIC_FLAG_QUAD 0x2         // NVM_IO_QUAD_ACCESS
#define NVM_MAGIC_FLAG_ACCESS NVM_MAGIC_FLAG_DUAL

struct nvm_dev {
	char name[NVM_DISK_NAME_LEN];	/* Device name e.g. nvme0n1 */
	struct nvm_geo geo;		/* Device information */
	int fd;				/* Char device for IOCTL */
	atomic_cnt ref_cnt;		/* Reference counter */
	UT_hash_handle hh;		/* Handle for device registry */
};

struct nvm_vblock {
	struct nvm_dev *dev;
	uint64_t ppa;
	uint16_t flags;
};

#endif /* __NVM_H */
