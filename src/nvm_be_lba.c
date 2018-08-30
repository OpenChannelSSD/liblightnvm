/*
 * be_lba - Backend based on kernel support for NVM IOCTL and LBA for VECTOR IO
 *
 * Copyright (C) 2015-2017 Javier Gonzáles <javier@cnexlabs.com>
 * Copyright (C) 2015-2017 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
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
#include <liblightnvm.h>
#include <nvm_be.h>

#ifndef NVM_BE_LBA_ENABLED
struct nvm_be nvm_be_lba = {
	.id = NVM_BE_LBA,

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.idfy = nvm_be_nosys_idfy,
	.rprt = nvm_be_nosys_rprt,
	.gfeat = nvm_be_nosys_gfeat,
	.sfeat = nvm_be_nosys_sfeat,
	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.erase = nvm_be_nosys_erase,
	.write = nvm_be_nosys_write,
	.read = nvm_be_nosys_read,
	.copy = nvm_be_nosys_copy,
};
#else
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <nvm_be_ioctl.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

static int nvm_be_lba_rw(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		         void *data, int rw)
{
	const size_t sect_bytes = dev->geo.sector_nbytes;
	char *buf = (char *)data;
	uint64_t dev_lbas[NVM_NADDR_MAX];
	int nr_lbas[NVM_NADDR_MAX];
	uint64_t prev_lba;
	int i, idx = 0;

	dev_lbas[idx] = prev_lba = nvm_addr_gen2dev(dev, addrs[0]);
	nr_lbas[idx] = 1;

	for (i = 1; i < naddrs; i++) {
		const uint64_t lba_dev = nvm_addr_gen2dev(dev, addrs[i]);

		/* Sequential */
		if (prev_lba == lba_dev - 1) {
			nr_lbas[idx]++;
		} else {
			idx++;
			dev_lbas[idx] = lba_dev;
			nr_lbas[idx] = 1;
		}

		prev_lba = lba_dev;
	}

	for (i = 0; i < idx + 1; i++) {
		const off_t offset = nvm_addr_dev2off(dev, dev_lbas[i]);
		ssize_t res;

		if (rw)
			res = pwrite(dev->fd, buf, sect_bytes * nr_lbas[i], offset);
		else
			res = pread(dev->fd, buf, sect_bytes * nr_lbas[i], offset);

		if (res < 0)
			return res;

		buf += sect_bytes * nr_lbas[i];
	}
	return 0;

}

int nvm_be_lba_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *data, void *meta, uint16_t NVM_UNUSED(flags),
		  struct nvm_ret *NVM_UNUSED(ret))
{
	if (meta) {
		errno = ENOSYS;
		return -1;
	}

	return nvm_be_lba_rw(dev, addrs, naddrs, data, 0);
}

int nvm_be_lba_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *data, void *meta, uint16_t NVM_UNUSED(flags),
		  struct nvm_ret *NVM_UNUSED(ret))
{
	if (meta) {
		errno = ENOSYS;
		return -1;
	}

	return nvm_be_lba_rw(dev, addrs, naddrs, data, 1);
}

int nvm_be_lba_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		     uint16_t NVM_UNUSED(flags), struct nvm_ret *NVM_UNUSED(ret))
{
	int i, err;
	uint64_t range[2];

	for (i = 0; i < naddrs; i++) {
		uint64_t offset = nvm_addr_dev2off(dev, nvm_addr_gen2dev(dev, addrs[i]));
		uint64_t length = dev->geo.l.nsectr << dev->ssw;

		range[0] = offset;
		range[1] = length;

		err = ioctl(dev->fd, BLKDISCARD, &range);
		if (err)
			return err;
	}

	return 0;
}

struct nvm_dev *nvm_be_lba_open(const char *dev_path, int NVM_UNUSED(flags))
{
	return nvm_be_ioctl_open(dev_path, NVM_BE_IOCTL_WRITABLE);
}

struct nvm_be nvm_be_lba = {
	.id = NVM_BE_LBA,

	.open = nvm_be_lba_open,
	.close = nvm_be_ioctl_close,

	.idfy = nvm_be_ioctl_idfy,
	.rprt = nvm_be_ioctl_rprt,
	.gfeat = nvm_be_ioctl_gfeat,
	.sfeat = nvm_be_ioctl_sfeat,
	.sbbt = nvm_be_ioctl_sbbt,
	.gbbt = nvm_be_ioctl_gbbt,

	.erase = nvm_be_lba_erase,
	.write = nvm_be_lba_write,
	.read = nvm_be_lba_read,
	.copy = nvm_be_ioctl_copy,
};
#endif
