/*
 * vblock - Virtual block functions
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
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
#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

struct nvm_vblock* nvm_vblock_new(void)
{
	struct nvm_vblock *vblock = malloc(sizeof(*vblock));
	memset(vblock, 0, sizeof(*vblock));

	return vblock;
}

void nvm_vblock_free(struct nvm_vblock **vblock)
{
	if (!vblock || !*vblock)
		return;

	free(*vblock);
	*vblock = NULL;
}

void nvm_vblock_pr(struct nvm_vblock *vblock)
{
	printf("vblock: id(%lu), bppa(%lu), vlun_id(%d), owner_id(%d),"
		" nppas(%d), ppa_bitmap(%d), flags(%d)\n",
		vblock->id, vblock->bppa, vblock->vlun_id, vblock->owner_id,
		vblock->nppas, vblock->ppa_bitmap, vblock->flags);
}

uint64_t nvm_vblock_get_id(struct nvm_vblock *vblock)
{
	return vblock->id;
}

uint64_t nvm_vblock_get_bppa(struct nvm_vblock *vblock)
{
	return vblock->bppa;
}

uint32_t nvm_vblock_get_lun(struct nvm_vblock *vblock)
{
	return vblock->vlun_id;
}

uint32_t nvm_vblock_get_owner(struct nvm_vblock *vblock)
{
	return vblock->owner_id;
}

uint32_t nvm_vblock_get_nppas(struct nvm_vblock *vblock)
{
	return vblock->nppas;
}

uint16_t nvm_vblock_get_bitmap(struct nvm_vblock *vblock)
{
	return vblock->ppa_bitmap;
}

uint16_t nvm_vblock_get_flags(struct nvm_vblock *vblock)
{
	return vblock->flags;
}

int nvm_vblock_get(struct nvm_vblock *vblock, struct nvm_tgt *tgt)
{
	int ret;

	struct nvm_ioctl_vblock ctl;
	ctl.id = 0;
	ctl.bppa = 0;
	ctl.nppas = 0;
	ctl.ppa_bitmap = 0;
	ctl.vlun_id = 0;
	ctl.owner_id = 101;
	ctl.flags = NVM_PROV_RAND_LUN;

	ret = ioctl(tgt->fd, NVM_BLOCK_GET, &ctl);

	vblock->id = ctl.id;
	vblock->bppa = ctl.bppa;
	vblock->nppas = ctl.nppas;
	vblock->ppa_bitmap = ctl.ppa_bitmap;
	vblock->vlun_id = ctl.vlun_id;
	vblock->owner_id = ctl.owner_id;
	vblock->flags = ctl.flags;

	NVM_DEBUG("ret(%d), vlun_id(%d), id(%llu), bppa(%llu)\n",
		  ret, vblock->vlun_id, vblock->id, vblock->bppa);

	return ret;
}

int nvm_vblock_gets(struct nvm_vblock *vblock, struct nvm_tgt *tgt,
		    uint32_t lun)
{
	int ret;

	struct nvm_ioctl_vblock ctl;
	ctl.id = 0;
	ctl.bppa = 0;
	ctl.nppas = 0;
	ctl.ppa_bitmap = 0;
	ctl.vlun_id = lun;
	ctl.owner_id = 101;
	ctl.flags = NVM_PROV_SPEC_LUN;

	ret = ioctl(tgt->fd, NVM_BLOCK_GET, &ctl);

	vblock->id = ctl.id;
	vblock->bppa = ctl.bppa;
	vblock->nppas = ctl.nppas;
	vblock->ppa_bitmap = ctl.ppa_bitmap;
	vblock->vlun_id = ctl.vlun_id;
	vblock->owner_id = ctl.owner_id;
	vblock->flags = ctl.flags;

	NVM_DEBUG("ret(%d), vlun_id(%d), id(%llu), bppa(%llu)\n",
		  ret, vblock->vlun_id, vblock->id, vblock->bppa);

	return ret;
}

int nvm_vblock_put(struct nvm_vblock *vblock, struct nvm_tgt *tgt)
{
	int ret;

	struct nvm_ioctl_vblock ctl;
	ctl.id = vblock->id;
	ctl.bppa = vblock->bppa;
	ctl.nppas = vblock->nppas;
	ctl.ppa_bitmap = vblock->ppa_bitmap;
	ctl.vlun_id = vblock->vlun_id;
	ctl.owner_id = vblock->owner_id;
	ctl.flags = vblock->flags;

	ret = ioctl(tgt->fd, NVM_BLOCK_PUT, &ctl);

	NVM_DEBUG("ret(%d), block(%llu), bppa(%llu), lun(%d)\n",
		  ret, vblock->id, vblock->bppa, vblock->vlun_id);

	return ret;
}

/* Currently pread is used to submit IO, in the future this might be SPDK,
 * libnvme, IOCTL or some other means.
 */
ssize_t nvm_vblock_read(struct nvm_vblock *vblock, struct nvm_tgt *tgt,
                        void *buf, size_t count, size_t ppa_off, int flags)
{
	size_t current_ppa;
	ssize_t remaining;
	char *reader;

	struct nvm_fpage *fpage = &tgt->dev->fpage;	/* Grab geometry */
	const uint32_t read_page_size = fpage->sec_size;/* XXX(1) */
	const uint32_t pg_sec_ratio = read_page_size / fpage->sec_size;
	const uint32_t max_pages_read = fpage->max_sec_io / pg_sec_ratio;

	assert(count <= (vblock->nppas - ppa_off));

	NVM_DEBUG("count(%lu), blk_id(%llu), tgt(%d)", count, vblock->id,
		  tgt->fd);

	current_ppa = vblock->bppa + ppa_off;
	remaining = count;
	reader = (char*)buf;
	while (remaining > 0) {
		int pages_per_read;
		size_t bytes_per_read;
		ssize_t bytes_read;

		pages_per_read = (remaining > max_pages_read) ?
				 max_pages_read : remaining;
		bytes_per_read = pages_per_read * read_page_size;

		NVM_DEBUG("bytes_per_read(%lu)"
			  ", pages_per_read(%d)"
			  ", current_ppa(%lu)\n",
			  bytes_per_read, pages_per_read, current_ppa);
retry:
		bytes_read = pread(tgt->fd, reader, bytes_per_read,
				   current_ppa * 4096);
		if (bytes_read != bytes_per_read) {
			if (errno == EINTR)
				goto retry;
			return -1;
		}

		reader += bytes_per_read;
		remaining -= pages_per_read * pg_sec_ratio;
		current_ppa += pages_per_read;
	}

	return count;
}

/* Currently pwrite is used to submit IO, in the future this might be SPDK,
 * libnvme, IOCTL or some other means.
 */
ssize_t nvm_vblock_write(struct nvm_vblock *vblock, struct nvm_tgt *tgt,
			 const void *buf, size_t count, size_t ppa_off,
			 int flags)
{
	size_t current_ppa;
	ssize_t remaining;
	char *writer;

	struct nvm_fpage *fpage = &tgt->dev->fpage;	/* Grab geometry */
	uint32_t write_page_size = fpage->pln_pg_size;
	uint32_t pg_sec_ratio = write_page_size / fpage->sec_size;
	uint32_t max_pages_write = fpage->max_sec_io / pg_sec_ratio;

	NVM_DEBUG("vblock_id(%llu), count(%lu), ppa_off(%lu), nppas(%lu)\n",
		  vblock->id, count, ppa_off, nppas);

	assert(count <= (vblock->nppa - ppa_off));

	current_ppa = vblock->bppa + (ppa_off * 16);
	remaining = count;
	writer = (char*)buf;
	while (remaining > 0) {
		int pages_per_write;
		size_t bytes_per_write;
		ssize_t written;

		pages_per_write = (remaining > max_pages_write) ?
				  max_pages_write : remaining;
		bytes_per_write = pages_per_write * write_page_size;

		NVM_DEBUG("bytes_per_write(%lu), pages_per_write(%d), "
			  "current_ppa(%lu)\n",
			  bytes_per_write, pages_per_write, current_ppa);

		written = pwrite(tgt->fd, writer, bytes_per_write,
				 current_ppa * 4096);
		if (written != bytes_per_write) {
			NVM_DEBUG("FAILED: written(%lu) != bytes_per_write(%lu)",
				  written, bytes_per_write);

			/* TODO: Retry with another page, and another block */
			return 0;
		}

		writer += bytes_per_write;
		current_ppa += pages_per_write * pg_sec_ratio;
		remaining -= pages_per_write;
	}

	NVM_DEBUG("Wrote count(%lu) pages to block(%llu), tgt->fd(%d)",
		  count, vblock->id, tgt->fd);

	return count;
}
