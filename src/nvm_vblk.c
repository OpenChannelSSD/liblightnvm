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

struct nvm_vblk* nvm_vblk_alloc_set(struct nvm_dev *dev,
				    struct nvm_addr addrs[], int naddrs)
{
	struct nvm_vblk *vblk;
	const struct nvm_geo *geo = nvm_dev_attr_geo(dev);

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

		vblk->addrs[i].ppa = addrs[i].ppa;
	}

	vblk->naddrs = naddrs;
	vblk->dev = dev;
	vblk->pos_write = 0;
	vblk->pos_read = 0;
	vblk->nbytes = vblk->naddrs * geo->nplanes * geo->npages *
		       geo->nsectors * geo->sector_nbytes;

	return vblk;
}

struct nvm_vblk *nvm_vblk_alloc(struct nvm_dev *dev, struct nvm_addr addr)
{
	return nvm_vblk_alloc_set(dev, &addr, 1);
}

struct nvm_vblk *nvm_vblk_alloc_line(struct nvm_dev *dev, int ch_bgn,
                                     int ch_end, int lun_bgn, int lun_end,
                                     int blk)
{
	struct nvm_vblk *vblk;
	const struct nvm_geo *geo = nvm_dev_attr_geo(dev);
	
	vblk = nvm_vblk_alloc_set(dev, NULL, 0);
	if (!vblk)
		return NULL;	// Propagate errno

	for (int ch = ch_bgn; ch <= ch_end; ++ch) {
		for (int lun = lun_bgn; lun <= lun_end; ++lun) {
			vblk->addrs[vblk->naddrs].ppa = 0;
			vblk->addrs[vblk->naddrs].g.ch = ch;
			vblk->addrs[vblk->naddrs].g.lun = lun;
			vblk->addrs[vblk->naddrs].g.blk = blk;
			++(vblk->naddrs);
		}
	}

	vblk->nbytes = vblk->naddrs * geo->nplanes * geo->npages *
		       geo->nsectors * geo->sector_nbytes;

	return vblk;
}

void nvm_vblk_free(struct nvm_vblk *vblk)
{
	free(vblk);
}

ssize_t nvm_vblk_erase(struct nvm_vblk *vblk)
{
	size_t nerr = 0;
	const struct nvm_geo *geo = nvm_dev_attr_geo(vblk->dev);
	const int nplanes = geo->nplanes;
	const int PMODE = vblk->dev->pmode;
	
	#pragma omp parallel for schedule(static) reduction(+:nerr)
	for (int i = 0; i < vblk->naddrs; ++i) {
		struct nvm_addr addrs[nplanes];
		ssize_t err;

		for (int i = 0; i < nplanes; ++i) {
			addrs[i].ppa = vblk->addrs[i].ppa;
			addrs[i].g.pl = i % nplanes;
		}

		err = nvm_addr_erase(vblk->dev, addrs, nplanes, PMODE, NULL);
		if (err) {
			NVM_DEBUG("FAILED: nvm_addr_erase err(%ld)", err);
			++nerr;
		}
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	return vblk->nbytes;
}

ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf, size_t count,
			size_t offset)
{
	size_t nerr = 0;
	const int PMODE = nvm_dev_attr_pmode(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_attr_geo(vblk->dev);

	const int alignment = geo->nplanes * geo->nsectors * geo->sector_nbytes;

	const size_t bgn = offset / alignment;
	const size_t end = bgn + (count / alignment);

	const int NVM_OP_NADDR = geo->nplanes * geo->nsectors;
	const int NVM_CMD_NADDR = NVM_OP_NADDR;

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

	#pragma omp parallel num_threads(vblk->naddrs) reduction(+:nerr)
	{
		const int tid = omp_get_thread_num();

		#pragma omp barrier
		for (size_t spg = bgn + tid; spg < end; spg += vblk->naddrs) {
			struct nvm_addr addrs[NVM_CMD_NADDR];
			const char *data_off;

			if (buf)
				data_off = data + spg * geo->sector_nbytes * NVM_CMD_NADDR - offset;
			else
				data_off = data;

			// channels X luns X pages
			int idx = spg % vblk->naddrs;
			int vpg = (spg / vblk->naddrs) % geo->npages;

			// Unroll: nplane X nsector
			for (int i = 0; i < NVM_CMD_NADDR; ++i) {
				addrs[i].ppa = vblk->addrs[idx].ppa;
				addrs[i].g.pg = vpg;
				addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
				addrs[i].g.sec = i % geo->nsectors;
			}

			ssize_t err = nvm_addr_write(vblk->dev, addrs,
						     NVM_CMD_NADDR, data_off,
						     NULL, PMODE, NULL);
			if (err) {
				NVM_DEBUG("FAILED: nvm_addr_write e(%ld)", err);
				++nerr;
			}
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
	const int PMODE = nvm_dev_attr_pmode(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_attr_geo(vblk->dev);

	const int alignment = geo->nplanes * geo->nsectors * geo->sector_nbytes;

	const size_t bgn = offset / alignment;
	const size_t end = bgn + (count / alignment);

	const int NVM_OP_NADDR = geo->nplanes * geo->nsectors;
	const int NVM_CMD_NADDR = NVM_OP_NADDR;

	if (offset + count > vblk->nbytes) {			// Check bounds
		errno = EINVAL;
		return -1;
	}

	if ((count % alignment) || (offset % alignment)) {	// Check align
		errno = EINVAL;
		return -1;
	}

	#pragma omp parallel num_threads(vblk->naddrs) reduction(+:nerr)
	{
		const int tid = omp_get_thread_num();

		#pragma omp barrier
		for (size_t spg = bgn + tid; spg < end; spg += vblk->naddrs) {
			struct nvm_addr addrs[NVM_CMD_NADDR];
			char *buf_off;

			buf_off = buf + spg * geo->sector_nbytes * NVM_CMD_NADDR - offset;

			// channels X luns X pages
			int idx = spg % vblk->naddrs;
			int vpg = (spg / vblk->naddrs) % geo->npages;

			// Unroll: nplane X nsector
			for (int i = 0; i < NVM_CMD_NADDR; ++i) {
				addrs[i].ppa = vblk->addrs[idx].ppa;
				addrs[i].g.pg = vpg;
				addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
				addrs[i].g.sec = i % geo->nsectors;
			}

			ssize_t err = nvm_addr_read(vblk->dev, addrs,
						     NVM_CMD_NADDR, buf_off,
						     NULL, PMODE, NULL);
			if (err) {
				NVM_DEBUG("FAILED: nvm_addr_write e(%ld)", err);
				++nerr;
			}
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

struct nvm_addr *nvm_vblk_attr_addrs(struct nvm_vblk *vblk)
{
	return vblk->addrs;
}

int nvm_vblk_attr_naddrs(struct nvm_vblk *vblk)
{
	return vblk->naddrs;
}

size_t nvm_vblk_attr_nbytes(struct nvm_vblk *vblk)
{
	return vblk->nbytes;
}

size_t nvm_vblk_attr_pos_read(struct nvm_vblk *vblk)
{
	return vblk->pos_read;
}

size_t nvm_vblk_attr_pos_write(struct nvm_vblk *vblk)
{
	return vblk->pos_write;
}

void nvm_vblk_pr(struct nvm_vblk *vblk)
{
	printf("vblk {\n");
	printf(" nbytes(%lub:%luMb),\n", vblk->nbytes, vblk->nbytes >> 20);
	printf("}\n");
	printf("vblk-"); nvm_addrs_pr(vblk->addrs, vblk->naddrs);
}

