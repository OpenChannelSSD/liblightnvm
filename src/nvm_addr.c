/*
 * addr - PPA address functions
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

void nvm_addr_pr(struct nvm_addr addr)
{
	printf("(%016lu){ ch(%02d), lun(%02d), pl(%d), "
	       "blk(%04d), pg(%03d), sec(%d) }\n",
	       addr.ppa, addr.g.ch, addr.g.lun, addr.g.pl,
	       addr.g.blk, addr.g.pg, addr.g.sec);
}

ssize_t nvm_addr_read(struct nvm_dev *dev, struct nvm_addr list[], int len,
		      void* buf)
{
	struct nvm_ioctl_dev_pio ctl;
	int ret;

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = NVM_MAGIC_OPCODE_READ;
	ctl.flags = NVM_MAGIC_FLAG_ACCESS;
	ctl.nppas = len;
	ctl.ppas = (uint64_t)list;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = dev->geo.nbytes * len;

	ret = ioctl(dev->fd, NVM_DEV_PIO, &ctl);
	if (ret || ctl.result || ctl.status) {
		NVM_DEBUG("ret(%d)\n", ret);
		NVM_DEBUG("result(0x%x)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", ctl.status);
	}
	if (ret) {
		return ret;
	}
	if (ctl.result) {
		return ctl.result;
	}

	return ctl.status;
}

ssize_t nvm_addr_write(struct nvm_dev *dev, struct nvm_addr list[], int len,
		       const void* buf)
{
	struct nvm_ioctl_dev_pio ctl;
	int ret;

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = NVM_MAGIC_OPCODE_WRITE;
	ctl.flags = NVM_MAGIC_FLAG_ACCESS;
	ctl.nppas = len;
	ctl.ppas = (uint64_t)list;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = dev->geo.nbytes * len;

	ret = ioctl(dev->fd, NVM_DEV_PIO, &ctl);
	if (ret || ctl.result || ctl.status) {
		NVM_DEBUG("ret(%d)\n", ret);
		NVM_DEBUG("result(0x%x)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", ctl.status);
	}
	if (ret) {
		return ret;
	}
	if (ctl.result) {
		return ctl.result;
	}

	return ctl.status;
}

