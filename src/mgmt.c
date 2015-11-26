/*
 * mgmt - management interface for Linux Open-Channel SSD (LightNVM)
 *
 * Copyright (C) 2015 Javier Gonzalez <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <liblightnvm.h>

#include "ioctl.h"

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
