/*
 * tgt - Target functions
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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <libudev.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

struct nvm_tgt_info* nvm_tgt_info_new(void)
{
	struct nvm_tgt_info *info = malloc(sizeof(*info));
	if (info)
		memset(info, 0, sizeof(*info));

	return info;
}

void nvm_tgt_info_free(struct nvm_tgt_info **info)
{
	if (!info)
		return;

	free(*info);
	*info = NULL;
}

void nvm_tgt_info_pr(struct nvm_tgt_info *info)
{
	printf("tgt_info: version(%d,%d,%d)"
		", dev_name(%s), tgt_name(%s)"
		", tgt_type_name(%s)\n",
		info->version[0], info->version[1], info->version[2],
		info->dev_name, info->tgt_name,
		info->tgt_type_name);
}

int nvm_tgt_info_fill(struct nvm_tgt_info *info, const char *tgt_name)
{
	struct udev *udev;
	struct udev_device *dev;

	udev = udev_new();
	if (!udev) {
		NVM_DEBUG("nvm_tgt_info_fill: Failed creating udev.\n");
		return -1;
	}

	dev = udev_dev_find(udev, "block", "disk", tgt_name);
	if (!dev) {
		NVM_DEBUG("nvm_tgt_info_fill: udev_dev_find failed\n");
		udev_unref(udev);
		return -1;
	}

	info->version[0] = 0;					// TODO: sysfs
	info->version[1] = 0;					// TODO: sysfs
	info->version[2] = 0;					// TODO: sysfs
	strncpy(info->dev_name, "nvme0n1", DISK_NAME_LEN);	// TODO: sysfs
	strncpy(info->tgt_name, tgt_name, DISK_NAME_LEN);
	strncpy(info->tgt_type_name,
		udev_device_get_sysattr_value(dev, "lightnvm/type"),
		NVM_TTYPE_NAME_MAX);

	udev_device_unref(dev);
	udev_unref(udev);

	return 0;
}

struct nvm_tgt* nvm_tgt_open(const char *tgt_name, int flags)
{
	char tgt_path[NVM_TGT_NAME_MAX];
	struct nvm_tgt *tgt;
	int err;

	tgt = malloc(sizeof(*tgt));
	if (!tgt)
		return NULL;

	err = nvm_tgt_info_fill(&tgt->info, tgt_name);
	if (err) {
		NVM_DEBUG("nvm_tgt_open: Failed retrieving tgt information.\n")
		free(tgt);
		return NULL;
	}

	tgt->dev = nvm_dev_open(tgt->info.dev_name);
	if (!tgt->dev) {
		NVM_DEBUG("nvm_tgt_open: Failed opening device(%s).\n",
			  tgt->info.dev_name)
		free(tgt);
		return NULL;
	}

	sprintf(tgt_path, "/dev/%s", tgt_name);
	tgt->fd = open(tgt_path, O_RDWR | O_DIRECT);
	if (tgt->fd < 0) {
		NVM_DEBUG("FAILED: open - tgt->fd(%d), tgt_path(%s)\n",
			  tgt->fd, tgt_path);

		nvm_dev_close(tgt->dev);
		free(tgt);

		return NULL;
	}

	return tgt;
}

int nvm_tgt_get_fd(struct nvm_tgt *tgt)
{
	return tgt->fd;
}

void nvm_tgt_close(struct nvm_tgt *tgt)
{
	NVM_DEBUG("tgt(%p), tgt->fd(%d)\n", tgt, tgt->fd);

	close(tgt->fd);
	nvm_dev_close(tgt->dev);
	free(tgt);
}

