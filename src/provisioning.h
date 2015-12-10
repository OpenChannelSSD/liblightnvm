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
#ifndef __LLNVM_PROVISION_H
#define __LLNVM_PROVISION_H

#include <stdint.h>

#include "ioctl.h"
#include "../util/uthash.h"

typedef struct nvm_ioctl_vblock VBLOCK;

static inline int get_npages_block(VBLOCK *vblock)
{
	return vblock->nppas / 4; //JAVIER: / NR_PLANES
}

/* provisioning */
int get_block(int tgt, uint32_t lun_id, VBLOCK *vblock);
// int get_block(int tgt, uint32_t lun_id, void *meta, uint16_t meta_size,
							// VBLOCK *vblock);
int get_block_meta(int tgt, uint64_t vblock_id, VBLOCK *vblock);
int put_block(int tgt, VBLOCK *vblock);

#endif

