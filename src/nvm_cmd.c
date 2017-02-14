/*
 * addr - Sector addressing functions for write, read, and meta-data print
 *        Block addressing functions for erase and mark
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <linux/nvme_ioctl.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

int nvm_cmd_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	int err;

	err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, cmd);
	if (ret) {
		ret->result = cmd->vuser.result;
		ret->status = cmd->vuser.status;
	}
	if (err || cmd->vuser.result) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_cmd_vadmin(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	int err;

	err = ioctl(dev->fd, NVME_NVM_IOCTL_ADMIN_VIO, cmd);
	if (ret) {
		ret->result = cmd->vadmin.result;
		ret->status = cmd->vadmin.status;
	}
	if (err || cmd->vadmin.result) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_cmd_user(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	int err;

	err = ioctl(dev->fd, NVME_IOCTL_SUBMIT_IO, cmd);
	if (err) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_cmd_admin(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	int err;

	err = ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, cmd);
	if (err) {
		errno = EIO;
		return -1;
	}

	return 0;
}

void nvm_cmd_pr(struct nvm_cmd *cmd)
{
	printf("nvm_cmd {\n");
	for (int i = 0; i < 16; ++i)
		printf(" cdw%02d("NVM_I32_FMT"),\n",
			i, NVM_I32_TO_STR(cmd->cdw[i]));
	printf("}\n");
}

