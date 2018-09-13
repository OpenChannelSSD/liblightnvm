/*
 * be_lbd - IOCTL be using Linux Block Device (LBD) for read, write and erase
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

#ifndef NVM_BE_LBD_ENABLED
struct nvm_be nvm_be_lbd = {
	.id = NVM_BE_LBD,
	.name = "NVM_BE_LBD",

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.idfy = nvm_be_nosys_idfy,
	.rprt = nvm_be_nosys_rprt,
	.gfeat = nvm_be_nosys_gfeat,
	.sfeat = nvm_be_nosys_sfeat,
	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.scalar_erase = nvm_be_nosys_scalar_erase,
	.scalar_write = nvm_be_nosys_scalar_write,
	.scalar_read = nvm_be_nosys_scalar_read,

	.vector_erase = nvm_be_nosys_vector_erase,
	.vector_write = nvm_be_nosys_vector_write,
	.vector_read = nvm_be_nosys_vector_read,
	.vector_copy = nvm_be_nosys_vector_copy,

	.async_init = nvm_be_nosys_async_init,
	.async_term = nvm_be_nosys_async_term,
	.async_poke = nvm_be_nosys_async_poke,
	.async_wait = nvm_be_nosys_async_wait,
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

static int nvm_be_lbd_scalar_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
				   int naddrs, uint16_t NVM_UNUSED(flags),
				   struct nvm_ret *NVM_UNUSED(ret))
{
	for (int i = 0; i < naddrs; i++) {
		uint64_t range[2];
		int err;
		
		range[0] = nvm_addr_gen2off(dev, addrs[i]);
		range[1] = dev->geo.l.nsectr << dev->ssw;

		err = ioctl(dev->fd, BLKDISCARD, &range);
		if (err) {
			NVM_DEBUG("FAILED: BLKDISCARD, err: %d, %s", err,
				  strerror(errno));
			// Propagate errno
			return -1;
		}
	}

	return 0;
}

int nvm_be_lbd_scalar_read(struct nvm_dev *dev, struct nvm_addr addr,
			   int naddrs, void *data, void *meta,
			   uint16_t NVM_UNUSED(flags),
			   struct nvm_ret *NVM_UNUSED(ret))
{
	const off_t offset = nvm_addr_gen2off(dev, addr);
	ssize_t res;

	if (meta) {
		NVM_DEBUG("FAILED: LBD read with meta is not supported");
		errno = ENOSYS;
		return -1;
	}

	res = pread(dev->fd, data, dev->geo.l.nbytes * naddrs, offset);
	if (res < 0) {
		NVM_DEBUG("FAILED: res: %zd, errno: %s", res, strerror(errno));
		// Propagate errno
		return -1;
	}

	return 0;
}

int nvm_be_lbd_scalar_write(struct nvm_dev *dev, struct nvm_addr addr,
			    int naddrs, const void *data, const void *meta,
			    uint16_t NVM_UNUSED(flags),
			    struct nvm_ret *NVM_UNUSED(ret))
{
	const off_t offset = nvm_addr_gen2off(dev, addr);
	ssize_t res;

	if (meta) {
		NVM_DEBUG("FAILED: LBD write with meta is not supported");
		errno = ENOSYS;
		return -1;
	}

	res = pwrite(dev->fd, data, dev->geo.l.nbytes * naddrs, offset);
	if (res < 0) {
		NVM_DEBUG("FAILED: res: %zd, errno: %s", res, strerror(errno));
		// Propagate errno
		return -1;
	}

	return 0;
}

struct nvm_dev *nvm_be_lbd_open(const char *dev_path, int NVM_UNUSED(flags))
{
	struct nvm_dev *dev;
	
	dev = nvm_be_ioctl_open(dev_path, NVM_BE_IOCTL_WRITABLE);
	if (!dev) {
		NVM_DEBUG("FAILED: opening via IOCTL_WRITABLE");
		// Propagate errno
		return NULL;
	}

	return dev;
}

struct nvm_be nvm_be_lbd = {
	.id = NVM_BE_LBD,
	.name = "NVM_BE_LBD",

	.open = nvm_be_lbd_open,
	.close = nvm_be_ioctl_close,

	.idfy = nvm_be_ioctl_idfy,
	.rprt = nvm_be_ioctl_rprt,
	.gfeat = nvm_be_ioctl_gfeat,
	.sfeat = nvm_be_ioctl_sfeat,
	.sbbt = nvm_be_ioctl_sbbt,
	.gbbt = nvm_be_ioctl_gbbt,

	.scalar_erase = nvm_be_lbd_scalar_erase,
	.scalar_write = nvm_be_lbd_scalar_write,
	.scalar_read = nvm_be_lbd_scalar_read,

	.vector_erase = nvm_be_ioctl_vector_erase,
	.vector_write = nvm_be_ioctl_vector_write,
	.vector_read = nvm_be_ioctl_vector_read,
	.vector_copy = nvm_be_ioctl_vector_copy,

	.async_init = nvm_be_nosys_async_init,
	.async_term = nvm_be_nosys_async_term,
	.async_poke = nvm_be_nosys_async_poke,
	.async_wait = nvm_be_nosys_async_wait,
};
#endif
