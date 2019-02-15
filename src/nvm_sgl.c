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
#include <bsd/sys/queue.h>

#include <liblightnvm_spec.h>
#include <nvm_dev.h>
#include <nvm_sgl.h>
#include <nvm_cmd.h>

struct nvm_sgl_pool *nvm_sgl_pool_create(struct nvm_dev *dev)
{
	struct nvm_sgl_pool *pool = calloc(1, sizeof(*pool));
	if (!pool) {
		return NULL;
	}

	pool->dev = dev;

	return pool;
}

void nvm_sgl_pool_destroy(struct nvm_sgl_pool *pool)
{
	struct nvm_dev *dev = pool->dev;
	struct nvm_sgl *sgl;

	while (!SLIST_EMPTY(&pool->free_list)) {
		sgl = SLIST_FIRST(&pool->free_list);
		SLIST_REMOVE_HEAD(&pool->free_list, free_list);
		nvm_sgl_destroy(dev, sgl);
	}

	free(pool);

}

struct nvm_sgl *nvm_sgl_create(struct nvm_dev *dev, int hint)
{
	size_t dsize = sizeof(struct nvm_nvme_sgl_descriptor);

	struct nvm_sgl *sgl;
	if (NULL == (sgl = calloc(1, sizeof(*sgl)))) {
		return NULL;
	}

	if (hint) {
		sgl->descriptors = nvm_buf_alloc(dev, hint * dsize, NULL);
		if (!sgl->descriptors) {
			free(sgl);
			return NULL;
		}
	}

	sgl->nalloc = hint;
	sgl->indirect = nvm_buf_alloc(dev, dsize, NULL);

	return sgl;
}

struct nvm_sgl *nvm_sgl_alloc(struct nvm_sgl_pool *pool)
{
	struct nvm_sgl *sgl;

	if (NULL != (sgl = SLIST_FIRST(&pool->free_list))) {
		SLIST_REMOVE_HEAD(&pool->free_list, free_list);
		return sgl;
	}

	return nvm_sgl_create(pool->dev, 1);
}

void nvm_sgl_destroy(struct nvm_dev *dev, struct nvm_sgl *sgl)
{
	nvm_buf_free(dev, sgl->descriptors);
	nvm_buf_free(dev, sgl->indirect);
	free(sgl);
}

void nvm_sgl_reset(struct nvm_sgl *sgl)
{
	sgl->ndescr = 0;
	sgl->len = 0;
}

void nvm_sgl_free(struct nvm_sgl_pool *pool, struct nvm_sgl *sgl)
{
	sgl->ndescr = 0;
	sgl->len = 0;
	SLIST_INSERT_HEAD(&pool->free_list, sgl, free_list);
}


int nvm_sgl_add(struct nvm_dev *dev, struct nvm_sgl *sgl, void *addr,
	size_t len)
{
	struct nvm_nvme_sgl_descriptor *d;

	/**
	 * FIXME(klaus.jensen): currently we only support one Last Segment
	 *  descriptor (one 4K page).
	 */
	if (sgl->ndescr == 256) {
		NVM_DEBUG("max 256 SGL descriptors supported");
		errno = EINVAL;
		return -1;
	}

	if (sgl->ndescr == sgl->nalloc) {
		size_t nbytes;
		sgl->nalloc = sgl->nalloc ? 2 * sgl->nalloc : 1;
		nbytes = sgl->nalloc * sizeof(struct nvm_nvme_sgl_descriptor);
		sgl->descriptors = nvm_buf_realloc(dev, sgl->descriptors, nbytes, NULL);
		if (!sgl->descriptors) {
			return -1;
		}
	}

	d = &sgl->descriptors[sgl->ndescr];
	d->unkeyed.type = NVM_NVME_SGL_DESCR_TYPE_DATA_BLOCK;
	d->unkeyed.len = len;

	uint64_t phys;

	if (nvm_buf_vtophys(dev, addr, &phys)) {
		return -1;
	}

	d->addr = phys;

	sgl->len += len;
	++sgl->ndescr;

	return 0;
}
