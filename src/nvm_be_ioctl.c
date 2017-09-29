/*
 * be_ioctl - Backend using for kernels with NVME_IOCTL_* & NVME_NVM_IOCTL_*
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

#ifndef NVM_BE_IOCTL_ENABLED
#include <liblightnvm.h>
#include <nvm_be.h>
struct nvm_be nvm_be_ioctl = {
	.id = NVM_BE_IOCTL,

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.user = nvm_be_nosys_user,
	.admin = nvm_be_nosys_admin,

	.vuser = nvm_be_nosys_vuser,
	.vadmin = nvm_be_nosys_vadmin
};
#else
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/lightnvm.h>
#include <linux/nvme_ioctl.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_be_ioctl.h>
#include <nvm_dev.h>
#include <nvm_debug.h>

int nvm_be_ioctl_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd,
		       struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, cmd);

	if (ret) {
		ret->result = cmd->vuser.result;
		ret->status = cmd->vuser.status;
	}

	if (err == -1)
		return err;		// Propagate errno from IOCTL error

	if (cmd->vuser.result) {	// Construct errno on cmd error
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_be_ioctl_vadmin(struct nvm_dev *dev, struct nvm_cmd *cmd,
			struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_ADMIN_VIO, cmd);

	if (ret) {
		ret->result = cmd->vadmin.result;
		ret->status = cmd->vadmin.status;
	}

	if (err == -1)
		return err;		// Propagate errno from IOCTL error

	if (cmd->vadmin.result) {	// Construct errno on cmd error
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_be_ioctl_user(struct nvm_dev *dev, struct nvm_cmd *cmd,
		      struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_IOCTL_SUBMIT_IO, cmd);

	if (ret) {
		ret->result = 0x0;
		ret->status = 0x0;
	}

	if (err == -1)
		return err;		// Propagate errno from IOCTL error

	return 0;
}

int nvm_be_ioctl_admin(struct nvm_dev *dev, struct nvm_cmd *cmd,
		       struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, cmd);

	if (ret) {
		ret->result = cmd->admin.result;
		ret->status = 0x0;
	}

	if (err == -1)
		return err;		// Propagate errno from IOCTL error

	if (cmd->vadmin.result) {	// Construct errno on cmd error
		errno = EIO;
		return -1;
	}

	return 0;
}

struct nvm_dev *nvm_be_ioctl_open(const char *dev_path, int flags)
{
	struct nvm_dev *dev = NULL;
	int err;
	
	if (strlen(dev_path) > NVM_DEV_PATH_LEN) {
		NVM_DEBUG("FAILED: Device path too long\n");
		errno = EINVAL;
		return NULL;
	}

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		NVM_DEBUG("FAILED: allocating 'struct nvm_dev'\n");
		return NULL;	// Propagate errno from malloc
	}

	strncpy(dev->path, dev_path, NVM_DEV_PATH_LEN);
	strncpy(dev->name, dev_path+5, NVM_DEV_NAME_LEN);

	switch (flags) {
	case NVM_BE_IOCTL_WRITABLE:
		dev->fd = open(dev->path, O_RDWR | O_DIRECT);
		break;

	default:
		dev->fd = open(dev->path, O_RDONLY);
		break;
	}
	if (dev->fd < 0) {
		NVM_DEBUG("FAILED: open(dev->path(%s)), dev->fd(%d)\n",
			  dev->path, dev->fd);

		free(dev);

		return NULL;	// Propagate errno from open
	}

	err = nvm_be_populate(dev, nvm_be_ioctl_vadmin);
	if (err) {
		NVM_DEBUG("FAILED: _ioctl_fill_geo, err(%d)", err);

		close(dev->fd);
		free(dev);

		return NULL;
	}

	return dev;
}

void nvm_be_ioctl_close(struct nvm_dev *dev)
{
	close(dev->fd);
}

struct nvm_be nvm_be_ioctl = {
	.id = NVM_BE_IOCTL,

	.open = nvm_be_ioctl_open,
	.close = nvm_be_ioctl_close,

	.user = nvm_be_ioctl_user,
	.admin = nvm_be_ioctl_admin,

	.vuser = nvm_be_ioctl_vuser,
	.vadmin = nvm_be_ioctl_vadmin,
};
#endif

