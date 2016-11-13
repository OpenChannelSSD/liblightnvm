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

static ssize_t nvm_addr_cmd(struct nvm_dev *dev, struct nvm_addr list[],
				    int len, void *buf, uint16_t flags,
			    uint16_t opcode)
{
	struct nvm_ioctl_dev_pio ctl;
	int err;

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = opcode;
	ctl.flags = flags;
	ctl.nppas = len;
	ctl.ppas = len == 1 ? list[0].ppa : (uint64_t)list;
	ctl.addr = (uint64_t)buf;
	ctl.data_len = buf ? dev->geo.nbytes * len : 0;

	err = ioctl(dev->fd, NVM_DEV_PIO, &ctl);
#ifdef NVM_DEBUG_ENABLED
	if (err || ctl.result || ctl.status) {
		int i;

		NVM_DEBUG("WARN: err(%d), ctl.r(0x%x), ctl.s(%llu), naddr(%d):",
			  err, ctl.result, ctl.status, ctl.nppas);
		for (i = 0; i < len; ++i)
			nvm_addr_pr(list[i]);
	}
#endif
	if (err) {		// Give up on IOCTL errors
		errno = EIO;
		return -1;
	}

	switch (ctl.result) {
	case 0x0:	// All good
		return 0;
	case 0x4700:	// As good as it gets..
		return 0;

	default:	// We give up on everything else
		errno = EIO;
		return -1;
	}
}

ssize_t nvm_addr_erase(struct nvm_dev *dev, struct nvm_addr list[], int len,
		       uint16_t flags)
{
	return nvm_addr_cmd(dev, list, len, NULL, flags,
			    NVM_MAGIC_OPCODE_ERASE);
}

ssize_t nvm_addr_write(struct nvm_dev *dev, struct nvm_addr list[], int len,
		       const void *buf, uint16_t flags)
{
	char *cbuf = (char *)buf;

	return nvm_addr_cmd(dev, list, len, cbuf, flags,
			    NVM_MAGIC_OPCODE_WRITE);
}

ssize_t nvm_addr_read(struct nvm_dev *dev, struct nvm_addr list[], int len,
		      void *buf, uint16_t flags)
{
	return nvm_addr_cmd(dev, list, len, buf, flags, NVM_MAGIC_OPCODE_READ);
}

ssize_t nvm_addr_mark(struct nvm_dev *dev, struct nvm_addr list[], int len,
		      uint16_t flags)
{
	switch (flags) {
	case 0x0:
	case 0x1:
	case 0x2:
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return nvm_addr_cmd(dev, list, len, NULL, flags, 0xF1);
}

void nvm_addr_pr(struct nvm_addr addr)
{
	printf("(%016lu){ ch(%02d), lun(%02d), pl(%d), ",
	       addr.ppa, addr.g.ch, addr.g.lun, addr.g.pl);
	printf("blk(%04d), pg(%03d), sec(%d) }\n",
	       addr.g.blk, addr.g.pg, addr.g.sec);
}

