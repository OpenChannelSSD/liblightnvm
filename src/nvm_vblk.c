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
	vblock->ppa = 0;
	vblock->flags = 0;

	return vblock;
}

struct nvm_vblk* nvm_vblk_new_on_dev(NVM_DEV dev, uint64_t ppa)
{
	struct nvm_vblk *vblock = malloc(sizeof(*vblock));

	vblock->dev = dev;
	vblock->ppa = ppa;
	vblock->flags = 0;

	return vblock;
}

void nvm_vblk_free(struct nvm_vblk **vblk)
{
	if (!vblk || !*vblk)
		return;

	free(*vblk);
	*vblk = NULL;
}

void nvm_vblk_pr(struct nvm_vblk *vblk)
{
	struct nvm_addr addr;

	addr.ppa = vblk->ppa;

	printf("vblk {");
	printf("\n  dev(%p),", vblk->dev);
	printf("\n  flags(%u),", vblk->flags);
	printf("\n  ");
	nvm_addr_pr(addr);
	printf("}\n");
}

uint64_t nvm_vblk_attr_ppa(struct nvm_vblk *vblk)
{
	return vblk->ppa;
}

uint16_t nvm_vblk_attr_flags(struct nvm_vblk *vblk)
{
	return vblk->flags;
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

	vblock->ppa = ctl.ppa;
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
	ctl.ppa = vblock->ppa;
	
	ret = ioctl(vblock->dev->fd, NVM_DEV_BLOCK_PUT, &ctl);

	return ret;
}

ssize_t nvm_vblk_pread(struct nvm_vblk *vblk, void *buf, size_t pg)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int len = geo.nplanes * geo.nsectors;

	struct nvm_addr list[len];
	int i;

	for (i = 0; i < len; i++) {
		list[i].ppa = vblk->ppa;

		list[i].g.pg = pg;
		list[i].g.pl = (i / geo.nsectors) % geo.nplanes;
		list[i].g.sec = i % geo.nsectors;
	}

	return nvm_addr_read(vblk->dev, list, len, buf, NVM_MAGIC_FLAG_DEFAULT);
}

ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf,
                        size_t pg)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	const int len = geo.nplanes * geo.nsectors;

	struct nvm_addr list[len];
	int i;

	for (i = 0; i < len; i++) {
		list[i].ppa = vblk->ppa;

		list[i].g.pg = pg;
		list[i].g.pl = (i / geo.nsectors) % geo.nplanes;
		list[i].g.sec = i % geo.nsectors;
	}

	return nvm_addr_write(vblk->dev, list, len, buf, NVM_MAGIC_FLAG_DEFAULT);
}

ssize_t nvm_vblk_write(struct nvm_vblk *vblk, const void *buf)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	
	int buf_off = 0;
	int pg;

	for(pg = 0; pg < geo.npages; ++pg) {
		int err = nvm_vblk_pwrite(vblk, buf + buf_off, pg);
		if (err) {
			return -(pg+1);
		}
		buf_off += geo.vpg_nbytes;
	}

	return 0;
}

ssize_t nvm_vblk_read(struct nvm_vblk *vblk, void *buf)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblk->dev);
	
	int buf_off = 0;
	int pg;

	for(pg = 0; pg < geo.npages; ++pg) {
		int err = nvm_vblk_pread(vblk, buf + buf_off, pg);
		if (err) {
			return -(pg+1);
		}
		buf_off += geo.vpg_nbytes;
	}

	return 0;
}

ssize_t nvm_vblk_erase(struct nvm_vblk *vblk)
{
	const int len = nvm_dev_attr_nplanes(vblk->dev);
	struct nvm_addr list[len];
	int i;

	for (i = 0; i < len; ++i) {
		list[i].ppa = vblk->ppa;
		list[i].g.pl = i;
	}

	return nvm_addr_erase(vblk->dev, list, len, NVM_MAGIC_FLAG_DEFAULT);
}

