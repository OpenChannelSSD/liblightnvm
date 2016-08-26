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
	struct NVM_ADDR addr;

	addr.ppa = vblock->ppa;

	printf("vblock: ppa(%lu:0x%lx), flags(%u)\n", vblock->ppa, vblock->ppa,
	       vblock->flags);
	printf("\t{ch(%d), lun(%d), pl(%d), blk(%d), pg(%d), sec(%d)}\n",
	       addr.g.ch, addr.g.lun, addr.g.pl, addr.g.blk, addr.g.pg,
	       addr.g.sec);
}

uint64_t nvm_vblock_get_ppa(struct nvm_vblock *vblock)
{
	return vblock->ppa;
}

uint16_t nvm_vblock_get_flags(struct nvm_vblock *vblock)
{
	return vblock->flags;
}

int nvm_vblock_gets(NVM_VBLOCK vblock, NVM_TGT tgt, uint32_t ch, uint32_t lun)
{
	struct nvm_ioctl_vblock ctl;
	struct NVM_ADDR addr;
	int err;

	addr.ppa = 0;
	addr.g.lun = lun;

	memset(&ctl, 0, sizeof(ctl));
	ctl.ppa = addr.ppa;

	err = ioctl(tgt->fd, NVM_BLOCK_GET, &ctl);
	if (err) {
		return err;
	}

	vblock->ppa = ctl.ppa;
	vblock->tgt = tgt;

	return 0;
}

int nvm_vblock_get(struct nvm_vblock *vblock, struct nvm_tgt *tgt)
{
	return nvm_vblock_gets(vblock, tgt, 0, 0);
}

int nvm_vblock_put(struct nvm_vblock *vblock)
{
	struct nvm_ioctl_vblock ctl;
	int ret;

	memset(&ctl, 0, sizeof(ctl));
	ctl.ppa = vblock->ppa;
	
	ret = ioctl(vblock->tgt->fd, NVM_BLOCK_PUT, &ctl);

	return ret;
}

#define NPPAS 16
#define NPLANES 4
ssize_t nvm_vblock_read(struct nvm_vblock *vblock, void *buf, size_t count,
			size_t ppa_off, int flags)
{
	struct NVM_ADDR ppas[NPPAS];
	struct nvm_ioctl_io ctl;
	int i, ret;

	for (i = 0; i < NPPAS; i++) {
		struct NVM_ADDR ppa;

		ppa.ppa = vblock->ppa;
		ppa.g.sec = i % NPLANES;
		ppa.g.pl = i / NPLANES;

		ppas[i] = ppa;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = 0x92;	/* MAGIC NUMBER -- NVM_OP_PREAD */
	ctl.flags = 0x2;	/* MAGIC NUMBER -- NVM_IO_QUAD_ACCESS */
	ctl.nppas = NPPAS;

	ctl.ppas = (uint64_t)ppas;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = vblock->tgt->dev->info.pln_pg_size;

	ret = ioctl(vblock->tgt->fd, NVM_PIO, &ctl);
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

ssize_t nvm_vblock_write(struct nvm_vblock *vblock, const void *buf,
			 size_t count, size_t ppa_off, int flags)
{
	struct NVM_ADDR ppas[NPPAS];
	struct nvm_ioctl_io ctl;
	int i, ret;

	for (i = 0; i < NPPAS; i++) {
		struct NVM_ADDR ppa;

		ppa.ppa = vblock->ppa;
		ppa.g.sec = i % NPLANES;
		ppa.g.pl = i / NPLANES;

		ppas[i] = ppa;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = 0x91;	/* MAGIC NUMBER -- NVM_OP_PWRITE */
	ctl.flags = 0x2;	/* MAGIC NUMBER -- NVM_IO_QUAD_ACCESS */
	ctl.nppas = NPPAS;

	ctl.ppas = (uint64_t)ppas;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = vblock->tgt->dev->info.pln_pg_size;

	ret = ioctl(vblock->tgt->fd, NVM_PIO, &ctl);
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

