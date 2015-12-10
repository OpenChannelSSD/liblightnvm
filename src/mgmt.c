/*
 * mgmt - management interface for Linux Open-Channel SSD (LightNVM)
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>

static int nvm_execute_ioctl(int opcode, void *u)
{
	char dev[FILENAME_MAX] = NVM_CTRL_FILE;
	int ret, fd;

	fd = open(dev, O_WRONLY);
	if (fd < 0)
		return fd;

	ret = ioctl(fd, opcode, u);
	if (ret)
		return ret;

	close(fd);
	return 0;
}

int nvm_get_info(struct nvm_ioctl_info *u)
{
	return nvm_execute_ioctl(NVM_INFO, u);
}

int nvm_get_devices(struct nvm_ioctl_get_devices *u)
{
	return nvm_execute_ioctl(NVM_GET_DEVICES, u);
}

int nvm_create_target(struct nvm_ioctl_tgt_create *u)
{
	return nvm_execute_ioctl(NVM_DEV_CREATE_TGT, u);
}

int nvm_remove_target(struct nvm_ioctl_tgt_remove *u)
{
	return nvm_execute_ioctl(NVM_DEV_REMOVE_TGT, u);
}

int nvm_get_device_info(struct nvm_ioctl_dev_info *u)
{
	return nvm_execute_ioctl(NVM_DEV_GET_INFO, u);
}

int nvm_get_target_info(struct nvm_ioctl_tgt_info *u)
{
	return nvm_execute_ioctl(NVM_TGT_GET_INFO, u);
}
