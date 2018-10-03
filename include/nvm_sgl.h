/*
 * nvm_sgl - Internal header for handling scatter/gather lists
 *
 * Copyright (C) Klaus B. A. Jensen <klaus.jensen@cnexlabs.com>
 *
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

#ifndef __INTERNAL_NVM_SGL_H
#define __INTERNAL_NVM_SGL_H

#include <sys/types.h>
#include <stdint.h>

#include <bsd/sys/queue.h>

struct nvm_sgl_entry {
	void *addr;
	size_t len;

	STAILQ_ENTRY(nvm_sgl_entry) entries;
};

struct nvm_sgl {
	struct nvm_sgl_entry *curr;

	STAILQ_HEAD(, nvm_sgl_entry) headp;
};

void nvm_sgl_reset(struct nvm_sgl *sgl);

int nvm_sgl_next_sge(struct nvm_sgl *sgl, void **addr, uint32_t *len);

#endif /* __INTERNAL_NVM_SGL_H */
