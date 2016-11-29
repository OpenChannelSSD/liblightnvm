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
	if (!vblk)
		return NULL;

	vblk->dev = 0;
	vblk->addr.ppa = 0;
	vblk->pos_write = 0;
	vblk->pos_read = 0;

	return vblk;
}

struct nvm_vblk *nvm_vblk_new_on_dev(NVM_DEV dev, NVM_ADDR addr)
{
	struct nvm_vblk *vblk;

	vblk = malloc(sizeof(*vblk));
	if (!vblk)
		return NULL;

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

	const int len = geo.nplanes;
	struct nvm_addr list[len];
	int i;
	int PLANE_FLAG = 0x0;

	PLANE_FLAG = (geo.nplanes == 4) ? NVM_MAGIC_FLAG_QUAD : PLANE_FLAG;
	PLANE_FLAG = (geo.nplanes == 2) ? NVM_MAGIC_FLAG_DUAL : PLANE_FLAG;

	for (i = 0; i < len; ++i) {
		list[i].ppa = vblk->addr.ppa;
		list[i].g.pl = i;
	}

	return nvm_addr_erase(vblk->dev, list, len, PLANE_FLAG);
}

ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf,
			size_t count, size_t offset)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int len = geo.nplanes * geo.nsectors;
	const int align = len * geo.nbytes;
	const int vpg_offset = offset / align;
	size_t nbytes_written = 0;
	int PLANE_FLAG = 0x0;

	if ((count % align) || (offset % align)) {
		errno = EINVAL;
		return -1;
	}

	PLANE_FLAG = (geo.nplanes == 4) ? NVM_MAGIC_FLAG_QUAD : PLANE_FLAG;
	PLANE_FLAG = (geo.nplanes == 2) ? NVM_MAGIC_FLAG_DUAL : PLANE_FLAG;

	while (nbytes_written < count) {
		struct nvm_addr list[len];
		ssize_t err;
		int i;

		for (i = 0; i < len; i++) {
			list[i].ppa = vblk->addr.ppa;

			list[i].g.pg = (nbytes_written / align) + vpg_offset;
			list[i].g.sec = i % geo.nsectors;
			list[i].g.pl = (i / geo.nsectors) % geo.nplanes;
		}

		err = nvm_addr_write(vblk->dev, list, len, buf + nbytes_written,
                                     NULL, PLANE_FLAG);
		if (err) {	// errno set by `nvm_addr_write`
			return -1;
		}

		nbytes_written += align;
	}

	return 0;
}

ssize_t nvm_vblk_write(struct nvm_vblk *vblk, const void *buf, size_t count)
{
	ssize_t err;

	err = nvm_vblk_pwrite(vblk, buf, count, vblk->pos_write);
	if (err)
		return -1;	// errno set by nvm_vblk_pwrite / nvm_addr_write

	vblk->pos_write += count;

	return 0;
}

ssize_t nvm_vblk_pread(struct nvm_vblk *vblk, void *buf, size_t count,
		       size_t offset)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int len = geo.nplanes * geo.nsectors;
	const int align = len * geo.nbytes;
	const int vpg_offset = offset / align;
	size_t nbytes_read = 0;
	int PLANE_FLAG = 0x0;

	if ((count % align) || (offset % align)) {
		errno = EINVAL;
		return -1;
	}

	PLANE_FLAG = (geo.nplanes == 4) ? NVM_MAGIC_FLAG_QUAD : PLANE_FLAG;
	PLANE_FLAG = (geo.nplanes == 2) ? NVM_MAGIC_FLAG_DUAL : PLANE_FLAG;

	while (nbytes_read < count) {
		struct nvm_addr list[len];
		ssize_t err;
		int i;

		for (i = 0; i < len; i++) {
			list[i].ppa = vblk->addr.ppa;

			list[i].g.pg = (nbytes_read / align) + vpg_offset;
			list[i].g.sec = i % geo.nsectors;
			list[i].g.pl = (i / geo.nsectors) % geo.nplanes;
		}

		err = nvm_addr_read(vblk->dev, list, len, buf + nbytes_read,
				    NULL, PLANE_FLAG);
		if (err) {	// errno set by `nvm_addr_read`
			return -1;
		}

		nbytes_read += align;
	}

	return 0;
}

ssize_t nvm_vblk_read(struct nvm_vblk *vblk, void *buf, size_t count)
{
	ssize_t err;

	err = nvm_vblk_pread(vblk, buf, count, vblk->pos_read);
	if (err)
		return -1;	// errno set by nvm_vblk_pread / nvm_addr_read

	vblk->pos_read += count;

	return 0;
}

