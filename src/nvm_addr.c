/*
 * addr - Sector addressing functions for pr, erase, read, write
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

static ssize_t nvm_addr_io(struct nvm_dev *dev, struct nvm_addr list[], int len,
		    void* buf, uint16_t flags, uint16_t opcode)
{
	struct nvm_ioctl_dev_pio ctl;
	int ret;

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = opcode;
	ctl.flags = flags;
	ctl.nppas = len;
	ctl.ppas = len == 1 ? list[0].ppa : (uint64_t)list;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = buf ? dev->geo.nbytes * len : 0;

	ret = ioctl(dev->fd, NVM_DEV_PIO, &ctl);
#ifdef NVM_DEBUG_ENABLED
	if (ret || ctl.result || ctl.status) {
		int i;
		NVM_DEBUG("ret(%d)\n", ret);
		NVM_DEBUG("result(0x%x)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", ctl.status);
		for (i = 0; i < len; ++i) {
			printf("ERR: "); nvm_addr_pr(list[i]);
		}
	}
#endif
	if (ret) {
		return ret;
	}
	if (ctl.result) {
		return ctl.result;
	}

	return ctl.status;
}

ssize_t nvm_addr_erase(struct nvm_dev *dev, struct nvm_addr list[], int len,
		       uint16_t flags)
{
	return nvm_addr_io(dev, list, len, NULL, flags, NVM_MAGIC_OPCODE_ERASE);
}

ssize_t nvm_addr_write(struct nvm_dev *dev, struct nvm_addr list[], int len,
		       const void* cbuf, uint16_t flags)
{
	char *buf = (char*)cbuf;
	return nvm_addr_io(dev, list, len, buf, flags, NVM_MAGIC_OPCODE_WRITE);
}

ssize_t nvm_addr_read(struct nvm_dev *dev, struct nvm_addr list[], int len,
		      void* buf, uint16_t flags)
{
	return nvm_addr_io(dev, list, len, buf, flags, NVM_MAGIC_OPCODE_READ);
}

int nvm_dev_mark(struct nvm_dev *dev, struct nvm_addr addr, int type)
{
	struct nvm_geo geo = nvm_dev_attr_geo(dev);

	const int NPLANES = geo.nplanes;
	struct nvm_addr ppas[NPLANES];
	struct nvm_ioctl_dev_pio ctl;
	int i, ret;

	switch (type) {
		case 0x0:	/* MAGIC -- NVM_BLK_T_FREE aka "good" */
		case 0x1:	/* MAGIC -- NVM_BLK_T_BAD */
		case 0x2:	/* MAGIC -- NVM_BLK_T_GRWN_BAD */
			break;
		default:
			return -EINVAL;
	}

	for (i = 0; i < NPLANES; i++) {	/* Unroll over nplanes */
		struct nvm_addr ppa;

		ppa.ppa = addr.ppa;
		ppa.g.pg = 0;
		ppa.g.sec = 0;
		ppa.g.pl = i;

		ppas[i] = ppa;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = 0xf1;	/* MAGIC -- NVM_OP_PWRITE */
	ctl.flags = type;	/* MAGIC -- encoding bad-block type in flags */
	ctl.nppas = NPLANES;
	ctl.ppas = (uint64_t)ppas;

	ret = ioctl(dev->fd, NVM_DEV_PIO, &ctl);
	if (ret) {
		NVM_DEBUG("failed ret(%d)\n", ret);
		return ret;
	}

	if (ctl.result) {
		NVM_DEBUG("result(%u)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", (unsigned long long)ctl.status);
	}

	return ret;
}

void nvm_addr_pr(struct nvm_addr addr)
{
	printf("(%016lu){ ch(%02d), lun(%02d), pl(%d), "
	       "blk(%04d), pg(%03d), sec(%d) }\n",
	       addr.ppa, addr.g.ch, addr.g.lun, addr.g.pl,
	       addr.g.blk, addr.g.pg, addr.g.sec);
}

