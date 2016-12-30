/*
 * vblock - Virtual block functions
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
#include <nvm_omp.h>

struct nvm_vblk* nvm_vblk_alloc(struct nvm_dev *dev, struct nvm_addr addrs[],
				int naddrs)
{
	struct nvm_vblk *vblk;
	const struct nvm_geo *geo;
	
	if (naddrs > 128) {
		errno = EINVAL;
		return NULL;
	}

	geo = nvm_dev_get_geo(dev);
	if (!geo) {
		errno = EINVAL;
		return NULL;
	}

	vblk = malloc(sizeof(*vblk));
	if (!vblk) {
		errno = ENOMEM;
		return NULL;
	}

	for (int i = 0; i < naddrs; ++i) {
		if (nvm_addr_check(addrs[i], geo)) {
			errno = EINVAL;
			free(vblk);
			return NULL;
		}

		vblk->blks[i].ppa = addrs[i].ppa;
	}

	vblk->nblks = naddrs;
	vblk->dev = dev;
	vblk->pos_write = 0;
	vblk->pos_read = 0;
	vblk->nbytes = vblk->nblks * geo->nplanes * geo->npages *
		       geo->nsectors * geo->sector_nbytes;
	vblk->nthreads = naddrs;

	return vblk;
}

struct nvm_vblk *nvm_vblk_alloc_line(struct nvm_dev *dev, int ch_bgn,
                                     int ch_end, int lun_bgn, int lun_end,
                                     int blk)
{
	struct nvm_vblk *vblk;
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	
	vblk = nvm_vblk_alloc(dev, NULL, 0);
	if (!vblk)
		return NULL;	// Propagate errno

	for (int lun = lun_bgn; lun <= lun_end; ++lun) {
		for (int ch = ch_bgn; ch <= ch_end; ++ch) {
			vblk->blks[vblk->nblks].ppa = 0;
			vblk->blks[vblk->nblks].g.ch = ch;
			vblk->blks[vblk->nblks].g.lun = lun;
			vblk->blks[vblk->nblks].g.blk = blk;
			++(vblk->nblks);
		}
	}

	vblk->nbytes = vblk->nblks * geo->nplanes * geo->npages *
		       geo->nsectors * geo->sector_nbytes;
	vblk->nthreads = vblk->nblks;

	return vblk;
}

void nvm_vblk_free(struct nvm_vblk *vblk)
{
	free(vblk);
}

ssize_t nvm_vblk_erase(struct nvm_vblk *vblk)
{
	size_t nerr = 0;
	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);
	const int nplanes = geo->nplanes;
	const int PMODE = vblk->dev->pmode;
	
	#pragma omp parallel for schedule(static) reduction(+:nerr)
	for (int i = 0; i < vblk->nblks; ++i) {
		struct nvm_addr addrs[nplanes];
		ssize_t err;

		for (int pl = 0; pl < nplanes; ++pl) {
			addrs[pl].ppa = vblk->blks[i].ppa;
			addrs[pl].g.pl = pl;
		}

		err = nvm_addr_erase(vblk->dev, addrs, nplanes, PMODE, NULL);
		if (err)
			++nerr;
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	vblk->pos_write = 0;
	vblk->pos_read = 0;

	return vblk->nbytes;
}

ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf, size_t count,
			size_t offset)
{
	size_t nerr = 0;
	const int PMODE = nvm_dev_get_pmode(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const int alignment = geo->nplanes * geo->nsectors * geo->sector_nbytes;

	const size_t bgn = offset / alignment;
	const size_t end = bgn + (count / alignment);

	const int NVM_CMD_NADDR = geo->nplanes * geo->nsectors;

	const char *data;

	if (offset + count > vblk->nbytes) {			// Check bounds
		errno = EINVAL;
		return -1;
	}

	if ((count % alignment) || (offset % alignment)) {	// Check align
		errno = EINVAL;
		return -1;
	}

	if (buf) {	// Use user-supplied buffer
		data = buf;
	} else {	// Allocate and use a padding buffer
		data = nvm_buf_alloc(geo, NVM_CMD_NADDR * geo->sector_nbytes);
		if (!data) {
			errno = ENOMEM;
			return -1;
		}
	}

	#pragma omp parallel num_threads(vblk->nthreads) reduction(+:nerr)
	{
		const size_t tid = omp_get_thread_num();

		for (size_t spg = bgn + tid; spg < end; spg += vblk->nthreads) {
			struct nvm_addr addrs[NVM_CMD_NADDR];
			const char *data_off;
			struct nvm_ret ret = {};

			if (buf)
				data_off = data + (spg - bgn) * geo->sector_nbytes * NVM_CMD_NADDR;
			else
				data_off = data;

			int idx = spg % vblk->nblks;
			int vpg = (spg / vblk->nblks) % geo->npages;

			// Unroll: nplane X nsector
			for (int i = 0; i < NVM_CMD_NADDR; ++i) {
				addrs[i].ppa = vblk->blks[idx].ppa;
				addrs[i].g.pg = vpg;
				addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
				addrs[i].g.sec = i % geo->nsectors;
			}

			ssize_t err = nvm_addr_write(vblk->dev, addrs,
						     NVM_CMD_NADDR, data_off,
						     NULL, PMODE, &ret);
			if (err)
				++nerr;
		}
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	return count;
}

ssize_t nvm_vblk_write(struct nvm_vblk *vblk, const void *buf, size_t count)
{
	ssize_t nbytes = nvm_vblk_pwrite(vblk, buf, count, vblk->pos_write);

	if (nbytes < 0)
		return nbytes;		// Propagate errno
	
	vblk->pos_write += nbytes;	// All is good, increment write position

	return nbytes;			// Return number of bytes written
}

ssize_t nvm_vblk_pad(struct nvm_vblk *vblk)
{
	return nvm_vblk_write(vblk, NULL, vblk->nbytes - vblk->pos_write);
}

ssize_t nvm_vblk_pread(struct nvm_vblk *vblk, void *buf, size_t count,
		       size_t offset)
{
	size_t nerr = 0;
	const int PMODE = nvm_dev_get_pmode(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const int alignment = geo->nplanes * geo->nsectors * geo->sector_nbytes;

	const size_t bgn = offset / alignment;
	const size_t end = bgn + (count / alignment);

	const int NVM_CMD_NADDR = geo->nplanes * geo->nsectors;

	if (offset + count > vblk->nbytes) {			// Check bounds
		errno = EINVAL;
		return -1;
	}

	if ((count % alignment) || (offset % alignment)) {	// Check align
		errno = EINVAL;
		return -1;
	}

	#pragma omp parallel num_threads(vblk->nthreads) reduction(+:nerr)
	{
		const size_t tid = omp_get_thread_num();

		for (size_t spg = bgn + tid; spg < end; spg += vblk->nthreads) {
			struct nvm_addr addrs[NVM_CMD_NADDR];
			char *buf_off;
			struct nvm_ret ret = {};

			buf_off = buf + (spg - bgn) * geo->sector_nbytes * NVM_CMD_NADDR;

			int idx = spg % vblk->nblks;
			int vpg = (spg / vblk->nblks) % geo->npages;

			// Unroll: nplanes X nsectors
			for (int i = 0; i < NVM_CMD_NADDR; ++i) {
				addrs[i].ppa = vblk->blks[idx].ppa;
				addrs[i].g.pg = vpg;
				addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
				addrs[i].g.sec = i % geo->nsectors;
			}

			ssize_t err = nvm_addr_read(vblk->dev, addrs,
						     NVM_CMD_NADDR, buf_off,
						     NULL, PMODE, &ret);
			if (err)
				++nerr;
		}
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	return count;
}

ssize_t nvm_vblk_read(struct nvm_vblk *vblk, void *buf, size_t count)
{
	ssize_t nbytes = nvm_vblk_pread(vblk, buf, count, vblk->pos_read);

	if (nbytes < 0)
		return nbytes;		// Propagate `errno`

	vblk->pos_read += nbytes;	// All is good, increment read position

	return nbytes;			// Return number of bytes read
}

struct nvm_addr *nvm_vblk_get_addrs(struct nvm_vblk *vblk)
{
	return vblk->blks;
}

int nvm_vblk_get_naddrs(struct nvm_vblk *vblk)
{
	return vblk->nblks;
}

size_t nvm_vblk_get_nbytes(struct nvm_vblk *vblk)
{
	return vblk->nbytes;
}

size_t nvm_vblk_get_pos_read(struct nvm_vblk *vblk)
{
	return vblk->pos_read;
}

size_t nvm_vblk_get_pos_write(struct nvm_vblk *vblk)
{
	return vblk->pos_write;
}

int nvm_vblk_get_nthreads(struct nvm_vblk *vblk)
{
	return vblk->nthreads;
}

int nvm_vblk_set_nthreads(struct nvm_vblk *vblk, int nthreads)
{
	if ((nthreads > vblk->nblks) || (nthreads < 1)) {
		errno = EINVAL;
		return -1;
	}

	vblk->nthreads = nthreads;

	return 0;
}

void nvm_vblk_pr(struct nvm_vblk *vblk)
{
	printf("vblk {\n");
	printf(" nbytes(%lub:%luMb),\n", vblk->nbytes, vblk->nbytes >> 20);
	printf("}\n");
	printf("vblk-"); nvm_addrs_pr(vblk->blks, vblk->nblks);
}

