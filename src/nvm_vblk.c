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
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

struct nvm_vblk *nvm_vblk_new(void)
{
	struct nvm_vblk *vblk;

	vblk = malloc(sizeof(*vblk));
	if (!vblk) {
		errno = ENOMEM;
		return NULL;
	}

	vblk->dev = 0;
	vblk->addr.ppa = 0;
	vblk->pos_write = 0;
	vblk->pos_read = 0;

	return vblk;
}

struct nvm_vblk *nvm_vblk_alloc(struct nvm_dev *dev, struct nvm_addr addr)
{
	struct nvm_vblk *vblk;

	vblk = malloc(sizeof(*vblk));
	if (!vblk) {
		errno = ENOMEM;
		return NULL;
	}

	vblk->dev = dev;
	vblk->addr = addr;
	vblk->pos_write = 0;
	vblk->pos_read = 0;

	return vblk;
}

void nvm_vblk_free(struct nvm_vblk *vblk)
{
	free(vblk);
}

void nvm_vblk_pr(struct nvm_vblk *vblk)
{
	printf("vblk {");
	printf("\n  dev(%p),", vblk->dev);
	printf("\n  ");
	nvm_addr_pr(vblk->addr);
	printf("}\n");
}

struct nvm_addr nvm_vblk_attr_addr(struct nvm_vblk *vblk)
{
	return vblk->addr;
}

int nvm_vblk_gets(struct nvm_vblk *vblock, struct nvm_dev *dev, uint32_t ch,
		  uint32_t lun)
{
	// We no longer have provisioning in the kernel so we fail until
	// we implement this somehow in the library
	errno = ENXIO;
	return -1;
}

int nvm_vblk_get(struct nvm_vblk *vblock, struct nvm_dev *dev)
{
	return nvm_vblk_gets(vblock, dev, 0, 0);
}

int nvm_vblk_put(struct nvm_vblk *vblock)
{
	// We no longer have provisioning in the kernel so we fail until
	// we implement this somehow in the library
	errno = ENXIO;
	return -1;
}

ssize_t nvm_vblk_erase(struct nvm_vblk *vblk)
{
	struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int naddrs = geo.nplanes;
	struct nvm_addr addrs[naddrs];
	const int PMODE = nvm_dev_attr_pmode(vblk->dev);

	for (int i = 0; i < naddrs; ++i) {
		addrs[i].ppa = vblk->addr.ppa;
		addrs[i].g.pl = i;
	}

	if (nvm_addr_erase(vblk->dev, addrs, naddrs, PMODE, NULL))
		return -1;		// Propagate errno

	return geo.vblk_nbytes;
}

ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf,
			size_t count, size_t offset)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int naddrs = geo.nplanes * geo.nsectors;
	struct nvm_addr addrs[naddrs];
	const int align = naddrs * geo.sector_nbytes;
	const int vpg_offset = offset / align;
	size_t nbytes_written = 0;
	const int PMODE = nvm_dev_attr_pmode(vblk->dev);

	if ((count % align) || (offset % align)) {
		errno = EINVAL;
		return -1;
	}

	while (nbytes_written < count) {
		for (int i = 0; i < naddrs; i++) {
			addrs[i].ppa = vblk->addr.ppa;

			addrs[i].g.pg = (nbytes_written / align) + vpg_offset;
			addrs[i].g.sec = i % geo.nsectors;
			addrs[i].g.pl = (i / geo.nsectors) % geo.nplanes;
		}

		if (nvm_addr_write(vblk->dev, addrs, naddrs,
				   buf + nbytes_written, NULL, PMODE,
				   NULL))
			return -1;	// Propagate errno

		nbytes_written += align;
	}

	return nbytes_written;
}

ssize_t nvm_vblk_write(struct nvm_vblk *vblk, const void *buf, size_t count)
{
	const ssize_t nbytes = nvm_vblk_pwrite(vblk, buf, count,
					       vblk->pos_write);

	if (nbytes < 0)
		return -1;		// Propagate errno

	vblk->pos_write += nbytes;

	return nbytes;
}

ssize_t nvm_vblk_pread(struct nvm_vblk *vblk, void *buf, size_t count,
		       size_t offset)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int len = geo.nplanes * geo.nsectors;
	struct nvm_addr list[len];
	const int align = len * geo.sector_nbytes;
	const int vpg_offset = offset / align;
	size_t nbytes_read = 0;
	const int PMODE = nvm_dev_attr_pmode(vblk->dev);

	if ((count % align) || (offset % align)) {
		errno = EINVAL;
		return -1;
	}

	while (nbytes_read < count) {
		for (int i = 0; i < len; i++) {
			list[i].ppa = vblk->addr.ppa;

			list[i].g.pg = (nbytes_read / align) + vpg_offset;
			list[i].g.sec = i % geo.nsectors;
			list[i].g.pl = (i / geo.nsectors) % geo.nplanes;
		}

		if (nvm_addr_read(vblk->dev, list, len, buf + nbytes_read, NULL,
				  PMODE, NULL))
			return -1;	// Progate errno

		nbytes_read += align;
	}

	return nbytes_read;
}

ssize_t nvm_vblk_read(struct nvm_vblk *vblk, void *buf, size_t count)
{
	const ssize_t nbytes = nvm_vblk_pread(vblk, buf, count, vblk->pos_read);

	if (nbytes < 0)
		return -1;		// Propagate errno

	vblk->pos_read += nbytes;

	return nbytes;
}

