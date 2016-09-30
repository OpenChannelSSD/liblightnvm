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
	vblock->dev = 0;
	vblock->ppa = 0;
	vblock->flags = 0;

	return vblock;
}

struct nvm_vblock* nvm_vblock_new_on_dev(NVM_DEV dev, uint64_t ppa)
{
	struct nvm_vblock *vblock = malloc(sizeof(*vblock));

	vblock->dev = dev;
	vblock->ppa = ppa;
	vblock->flags = 0;

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
	struct nvm_addr addr;

	addr.ppa = vblock->ppa;

	printf("vblock: ppa(%lu:0x%lx), flags(%u)\n", vblock->ppa, vblock->ppa,
	       vblock->flags);
	printf("\t{ch(%d), lun(%d), pl(%d), blk(%d), pg(%d), sec(%d)}\n",
	       addr.g.ch, addr.g.lun, addr.g.pl, addr.g.blk, addr.g.pg,
	       addr.g.sec);
}

uint64_t nvm_vblock_attr_ppa(struct nvm_vblock *vblock)
{
	return vblock->ppa;
}

uint16_t nvm_vblock_attr_flags(struct nvm_vblock *vblock)
{
	return vblock->flags;
}

int nvm_vblock_gets(struct nvm_vblock *vblock, struct nvm_dev *dev, uint32_t ch,
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

int nvm_vblock_get(struct nvm_vblock *vblock, struct nvm_dev *dev)
{
	return nvm_vblock_gets(vblock, dev, 0, 0);
}

int nvm_vblock_put(struct nvm_vblock *vblock)
{
	struct nvm_ioctl_dev_vblk ctl;
	int ret;

	memset(&ctl, 0, sizeof(ctl));
	ctl.ppa = vblock->ppa;
	
	ret = ioctl(vblock->dev->fd, NVM_DEV_BLOCK_PUT, &ctl);

	return ret;
}

ssize_t nvm_vblock_pread(struct nvm_vblock *vblock, void *buf, size_t count,
			 size_t ppa_off)
{
	struct nvm_dev *dev = vblock->dev;
	const int NPLANES = nvm_dev_attr_nplanes(dev);
	const int NPPAS_MAX = NPLANES * nvm_dev_attr_nsectors(dev);

	struct nvm_addr ppas[NPPAS_MAX];
	struct nvm_ioctl_dev_pio ctl;
	int i, ret;

	for (i = 0; i < NPPAS_MAX; i++) {
		struct nvm_addr ppa;

		ppa.ppa = vblock->ppa;
		ppa.g.pg = ppa_off;
		ppa.g.sec = i % NPLANES;
		ppa.g.pl = i / NPLANES;

		ppas[i] = ppa;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = 0x92;	/* MAGIC NUMBER -- NVM_OP_PREAD */
	ctl.flags = 0x2;	/* MAGIC NUMBER -- NVM_IO_QUAD_ACCESS */
	ctl.nppas = NPPAS_MAX;

	ctl.ppas = (uint64_t)ppas;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = vblock->dev->geo.vpage_nbytes;

	ret = ioctl(vblock->dev->fd, NVM_DEV_PIO, &ctl);
	if (ret) {
		NVM_DEBUG("failed ret(%d)\n", ret);
		return 0;
	}

	if (ctl.result) {
		NVM_DEBUG("result(%u)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", (unsigned long long)ctl.status);
	}

	return count;
}

ssize_t nvm_vblock_pwrite(struct nvm_vblock *vblock, const void *buf,
			  size_t count,
			  size_t ppa_off)
{
	struct nvm_dev *dev = vblock->dev;
	const int NPLANES = nvm_dev_attr_nplanes(dev);
	const int NPPAS_MAX = NPLANES * nvm_dev_attr_nsectors(dev);

	struct nvm_addr ppas[NPPAS_MAX];
	struct nvm_ioctl_dev_pio ctl;
	int i, ret;

	for (i = 0; i < NPPAS_MAX; i++) {
		struct nvm_addr ppa;

		ppa.ppa = vblock->ppa;
		ppa.g.pg = ppa_off;
		ppa.g.sec = i % NPLANES;
		ppa.g.pl = i / NPLANES;

		ppas[i] = ppa;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = 0x91;	/* MAGIC NUMBER -- NVM_OP_PWRITE */
	ctl.flags = 0x2;	/* MAGIC NUMBER -- NVM_IO_QUAD_ACCESS */
	ctl.nppas = NPPAS_MAX;

	ctl.ppas = (uint64_t)ppas;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = vblock->dev->geo.vpage_nbytes;

	ret = ioctl(vblock->dev->fd, NVM_DEV_PIO, &ctl);
	if (ret) {
		NVM_DEBUG("failed ret(%d)\n", ret);
		return 0;
	}

	if (ctl.result) {
		NVM_DEBUG("result(%u)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", (unsigned long long)ctl.status);
	}

	return count;
}

int nvm_vblock_write(struct nvm_vblock *vblock, const void *buf)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblock->dev);
	
	int buf_off = 0;
	int pg;

	for(pg=0; pg<geo.npages; ++pg) {
		int ret = nvm_vblock_pwrite(vblock, buf+buf_off, 1, pg);
		if (!ret) {
			return -(pg+1);
		}
		buf_off += geo.vpage_nbytes;
	}

	return 0;
}

int nvm_vblock_read(struct nvm_vblock *vblock, void *buf)
{
	const struct nvm_geo geo = nvm_dev_attr_geo(vblock->dev);
	
	int buf_off = 0;
	int pg;

	for(pg=0; pg<geo.npages; ++pg) {
		int ret = nvm_vblock_pread(vblock, buf+buf_off, 1, pg);
		if (!ret) {
			return -(pg+1);
		}
		buf_off += geo.vpage_nbytes;
	}

	return 0;
}

int nvm_vblock_erase(struct nvm_vblock *vblock)
{
	struct nvm_dev *dev = vblock->dev;
	const int NPLANES = nvm_dev_attr_nplanes(dev);

	struct nvm_addr ppas[NPLANES];
	struct nvm_ioctl_dev_pio ctl;
	int i, ret;

	for (i = 0; i < NPLANES; i++) {
		struct nvm_addr ppa;

		ppa.ppa = vblock->ppa;
		ppa.g.pl = i;

		ppas[i] = ppa;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = 0x90;	/* MAGIC NUMBER -- NVM_OP_PERASE */
	ctl.flags = 0x2;	/* MAGIC NUMBER -- NVM_IO_QUAD_ACCESS */
	ctl.nppas = NPLANES;

	ctl.ppas = (uint64_t)ppas;

	ret = ioctl(vblock->dev->fd, NVM_DEV_PIO, &ctl);
	if (ret) {
		NVM_DEBUG("failed ret(%d)\n", ret);
		return ret;
	}

	if (ctl.result) {
		NVM_DEBUG("result(%u)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", (unsigned long long)ctl.status);
	}

	return 0;
}
