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
#include <nvm_be_ioctl.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

int nvm_be_lba_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		    void *data, void *meta, uint16_t NVM_UNUSED(flags),
		    struct nvm_ret *NVM_UNUSED(ret))
{
	const size_t count = dev->geo.sector_nbytes;

	if (meta) {
		errno = ENOSYS;
		return -1;
	}
	
	for (int i = 0; i < naddrs; ++i) {
		const uint64_t lba = nvm_addr_dev2off(dev, addrs[i].ppa);
		char *buf = ((char *)data) + (i * count);
		ssize_t res = pread(dev->fd, buf, count, lba);

		if (res < 0)
			return -1;
	}

	return 0;
}

int nvm_be_lba_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *data, void *meta, uint16_t NVM_UNUSED(flags),
		  struct nvm_ret *NVM_UNUSED(ret))
{
	const size_t count = dev->geo.sector_nbytes;

	if (meta) {
		errno = ENOSYS;
		return -1;
	}

	for (int i = 0; i < naddrs; ++i) {
		const uint64_t lba = nvm_addr_dev2off(dev, addrs[i].ppa);
		char *buf = ((char *)data) + (i * count);
		ssize_t res = pwrite(dev->fd, buf, count, lba);

		if (res < 0)
			return -1;
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
	.sbbt = nvm_be_ioctl_sbbt,
	.gbbt = nvm_be_ioctl_gbbt,

	.erase = nvm_be_ioctl_erase,
	.write = nvm_be_lba_write,
	.read = nvm_be_lba_read,
	.copy = nvm_be_ioctl_copy,
};
#endif

