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

struct nvm_vblk* nvm_vblk_new(void)
{
	struct nvm_vblk *vblock = malloc(sizeof(*vblock));
	vblock->dev = 0;
	vblock->addr.ppa = 0;
	vblock->pos_write = 0;
	vblock->pos_read = 0;

	return vblock;
}

struct nvm_vblk* nvm_vblk_new_on_dev(NVM_DEV dev, NVM_ADDR addr)
{
	struct nvm_vblk *vblock = malloc(sizeof(*vblock));

	vblock->dev = dev;
	vblock->addr = addr;
	vblock->pos_write = 0;
	vblock->pos_read = 0;

	return vblock;
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
	struct nvm_ioctl_dev_vblk ctl;
	struct nvm_addr addr;
	int err;

	addr.ppa = 0;
	addr.g.lun = lun;

	memset(&ctl, 0, sizeof(ctl));
	ctl.ppa = addr.ppa;

	err = ioctl(dev->fd, NVM_DEV_BLOCK_GET, &ctl);
	if (err) {
		return err;
	}

	vblock->addr.ppa = ctl.ppa;
	vblock->dev = dev;

	return 0;
}

int nvm_vblk_get(struct nvm_vblk *vblock, struct nvm_dev *dev)
{
	return nvm_vblk_gets(vblock, dev, 0, 0);
}

int nvm_vblk_put(struct nvm_vblk *vblock)
{
	struct nvm_ioctl_dev_vblk ctl;
	int ret;

	memset(&ctl, 0, sizeof(ctl));
	ctl.ppa = vblock->addr.ppa;
	
	ret = ioctl(vblock->dev->fd, NVM_DEV_BLOCK_PUT, &ctl);

	return ret;
}

ssize_t nvm_vblk_erase(struct nvm_vblk *vblk)
{
	struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);

	const int len = geo.nplanes;
	struct nvm_addr list[len];
	int i;

	for (i = 0; i < len; ++i) {
		list[i].ppa = vblk->addr.ppa;
		list[i].g.pl = i;
	}

	return nvm_addr_erase(vblk->dev, list, len, NVM_MAGIC_FLAG_DEFAULT);
}

ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf,
                        size_t count, size_t offset)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int len = geo.nplanes * geo.nsectors;
	const int align = len * geo.nbytes;
	const int vpg_offset = offset / align;
	size_t nbytes_written = 0;

	if ((count % align) || (offset % align)) {
		errno = EINVAL;
		return -1;
	}

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
				     NVM_MAGIC_FLAG_DEFAULT);
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
	if (err) {		// errno set by nvm_vblk_pwrite / nvm_addr_write
		return -1;
	}

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

	if ((count % align) || (offset % align)) {
		errno = EINVAL;
		return -1;
	}

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
				     NVM_MAGIC_FLAG_DEFAULT);
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
	if (err) {		// errno set by nvm_vblk_pread / nvm_addr_read
		return -1;
	}

	vblk->pos_read += count;

	return 0;
}

