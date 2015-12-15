/*
 * provisioning - LightNVM get/put block API
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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
#ifndef __PROVISIONING_H
#define __PROVISIONING_H

#include <pthread.h>
#include <liblightnvm.h>

#include "../util/atomic.h"
#include "../util/uthash.h"

#define NVM_TGT_NAME_MAX DISK_NAME_LEN + 5	/* 5 = strlen(/dev/) */

struct nvm_fpage {
	uint32_t sec_size;
	uint32_t page_size;
	uint32_t pln_pg_size;
	uint32_t max_sec_io;
};

struct nvm_device {
	struct nvm_ioctl_dev_info info;	/* Device properties */
	struct nvm_fpage flash_page;	/* Calculated device flash page sizes */
	atomic_cnt ref_cnt;		/* Reference counter */
	UT_hash_handle hh;		/* hash handle for uthash */
};

struct nvm_target {
	uint32_t version[3];
	uint32_t reserved;
	char tgtname[DISK_NAME_LEN];		/* dev to expose target as */
	char tgttype[NVM_TTYPE_NAME_MAX];	/* target type name */
	struct nvm_device *dev;		/* Device associated */
	atomic_cnt ref_cnt;			/* Reference counter */
	UT_hash_handle hh;			/* hash handle for uthash */
};

struct nvm_target_map {
	struct nvm_target *tgt;
	int tgt_id;
	UT_hash_handle hh;			/* hash handle for uthash */
};

struct nvm_target_map *get_nvm_tgt_map(int tgt);

#endif /* __PROVISIONING_H */
