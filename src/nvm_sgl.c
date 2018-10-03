/*
 * sgl - Implementation of SGL helper functions
 *
 * Copyright (C) Klaus B. A. Jensen <klaus.jensen@cnexlabs.com>

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

#include <stdlib.h>
#include <nvm_sgl.h>
#include <nvm_cmd.h>

struct nvm_sgl *nvm_sgl_alloc()
{
	struct nvm_sgl *sgl = calloc(1, sizeof(*sgl));

	STAILQ_INIT(&sgl->headp);

	return sgl;
}

void nvm_sgl_add(struct nvm_sgl *sgl, void *addr, size_t len)
{
	struct nvm_sgl_entry *entry = calloc(1, sizeof(*entry));
	entry->addr = addr;
	entry->len = len;

	STAILQ_INSERT_TAIL(&sgl->headp, entry, entries);
}

void nvm_sgl_free(struct nvm_sgl *sgl)
{
	struct nvm_sgl_entry *entry = STAILQ_FIRST(&sgl->headp);
	struct nvm_sgl_entry *tmp;

	while (entry != NULL)	{
		tmp = STAILQ_NEXT(entry, entries);
		free(entry);
		entry = tmp;
	}

	free(sgl);
}

void nvm_sgl_reset(struct nvm_sgl *sgl)
{
	sgl->curr = STAILQ_FIRST(&sgl->headp);
}

int nvm_sgl_next_sge(struct nvm_sgl *sgl, void **address, uint32_t *length)
{
	struct nvm_sgl_entry *iov = sgl->curr;

	if (!iov) {
		*length = 0;
		*address = NULL;

		return 0;
	}

	*address = iov->addr;
	*length = iov->len;

	sgl->curr = STAILQ_NEXT(sgl->curr, entries);

	return 0;
}
