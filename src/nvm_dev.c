/*
 * dev - Device functions
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
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <libudev.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_util.h>
#include <nvm_debug.h>

static struct nvm_dev *devices = NULL;

int nvm_dev_geo_fill(struct nvm_geo *geo, const char *dev_name)
{
	struct udev *udev;
	struct udev_device *dev;

	udev = udev_new();
	if (!udev) {
		NVM_DEBUG("Failed creating udev for dev_name(%s)\n", dev_name);
		return -1;
	}

	/* Extract geometry from sysfs via libudev*/
	dev = udev_nvmdev_find(udev, dev_name);
	if (!dev) {
		NVM_DEBUG("Cannot find dev_name(%s)\n", dev_name);
		udev_unref(udev);
		return -1;
	}

	geo->nchannels = atoi(udev_device_get_sysattr_value(dev, "device/lightnvm/num_channels"));
	geo->nluns = atoi(udev_device_get_sysattr_value(dev, "device/lightnvm/num_luns"));
	geo->nplanes = atoi(udev_device_get_sysattr_value(dev, "device/lightnvm/num_planes"));
	geo->nblocks = atoi(udev_device_get_sysattr_value(dev, "device/lightnvm/num_blocks"));
	geo->npages = atoi(udev_device_get_sysattr_value(dev, "device/lightnvm/num_pages"));
	geo->nsectors = atoi(udev_device_get_sysattr_value(dev, "device/lightnvm/sec_per_pg"));
	geo->nbytes = atoi(udev_device_get_sysattr_value(dev, "device/lightnvm/hw_sector_size"));

	/* Derive total number of bytes on device */
	geo->tbytes = geo->nchannels * geo->nluns * geo->nplanes * \
		      geo->nblocks * geo->npages * geo->nsectors * geo->nbytes;

	/* Derive number of bytes occupied by a virtual block/page */
	geo->vblk_nbytes = geo->nplanes * geo->npages * geo->nsectors * geo->nbytes;
	geo->vpg_nbytes = geo->nplanes * geo->nsectors * geo->nbytes;

	udev_device_unref(dev);
	udev_unref(udev);

	return 0;
}

struct nvm_dev* nvm_dev_new(void)
{
	struct nvm_dev *dev;

	dev = malloc(sizeof(*dev));
	if (dev) {
		memset(dev, 0, sizeof(*dev));
		atomic_init(&dev->ref_cnt);
		atomic_set(&dev->ref_cnt, 0);
	}

	return dev;
}

void nvm_dev_free(struct nvm_dev **dev)
{
	if (!dev)
		return;

	free(*dev);
	*dev = NULL;
}

void nvm_dev_pr(struct nvm_dev* dev)
{
	printf("dev { name(%s), fd(%d) }\n", dev->name, dev->fd);
}

int nvm_dev_attr_nchannels(struct nvm_dev *dev)
{
	return dev->geo.nchannels;
}

int nvm_dev_attr_nluns(struct nvm_dev *dev)
{
	return dev->geo.nluns;
}

int nvm_dev_attr_nplanes(struct nvm_dev *dev)
{
	return dev->geo.nplanes;
}

int nvm_dev_attr_nblocks(struct nvm_dev *dev)
{
	return dev->geo.nblocks;
}

int nvm_dev_attr_npages(struct nvm_dev *dev)
{
	return dev->geo.npages;
}

int nvm_dev_attr_nsectors(struct nvm_dev *dev)
{
	return dev->geo.nsectors;
}

int nvm_dev_attr_nbytes(struct nvm_dev *dev)
{
	return dev->geo.nbytes;
}

int nvm_dev_attr_vblk_nbytes(struct nvm_dev *dev)
{
	return dev->geo.vblk_nbytes;
}

int nvm_dev_attr_vpage_nbytes(struct nvm_dev *dev)
{
	return dev->geo.vpg_nbytes;
}

struct nvm_geo nvm_dev_attr_geo(struct nvm_dev *dev)
{
	return dev->geo;
}

void nvm_geo_pr(struct nvm_geo geo)
{
	printf("geo {\n");
	printf(" nchannels(%lu), nluns(%lu), nplanes(%lu), nblocks(%lu),\n",
		geo.nchannels, geo.nluns, geo.nplanes, geo.nblocks);
	printf(" npages(%lu), nsectors(%lu), nbytes(%lu), \n",
		geo.npages, geo.nsectors, geo.nbytes);
	printf(" total_nbytes(%lub:%luMb)\n",
		geo.tbytes, geo.tbytes >> 20);
	printf(" vblk_nbytes(%lub:%luMb)\n",
		geo.vblk_nbytes, geo.vblk_nbytes >> 20);
	printf(" vpg_nbytes(%lub:%luKb)\n",
		geo.vpg_nbytes, geo.vpg_nbytes >> 10);
	printf("}\n");
}

struct nvm_dev* nvm_dev_open(const char *dev_name)
{
	char dev_path[NVM_DISK_NAME_LEN];
	struct nvm_dev *dev;
	int err;

	HASH_FIND_STR(devices, dev_name, dev);
	if (dev) {
		atomic_inc(&dev->ref_cnt);
		return dev;
	}

	dev = nvm_dev_new();
	if (!dev) {
		NVM_DEBUG("nvm_dev_open: nvm_dev_new failed.\n");
		return NULL;
	}

	strncpy(dev->name, dev_name, DISK_NAME_LEN);
	
	err = nvm_dev_geo_fill(&dev->geo, dev_name);
	if (err) {
		NVM_DEBUG("Failed nvm_dev_geo_fill err(%d)\n", err);
		nvm_dev_free(&dev);
		return NULL;
	}

	sprintf(dev_path, "/dev/%s", dev_name);
	dev->fd = open(dev_path, O_RDWR);
	if (dev->fd < 0) {
		NVM_DEBUG("Failed open dev_path(%s) dev->fd(%d)\n",
			  dev_path, dev->fd);

		nvm_dev_close(dev);
		free(dev);

		return NULL;
	}

	atomic_inc(&dev->ref_cnt);
	HASH_ADD_STR(devices, name, dev);

	return dev;
}

void nvm_dev_close(struct nvm_dev *dev)
{
	if (atomic_dec_and_test(&(dev)->ref_cnt)) {
		HASH_DEL(devices, dev);
		close(dev->fd);
		nvm_dev_free(&dev);
	}
}

int nvm_dev_mark(struct nvm_dev *dev, struct nvm_addr addr, int type)
{
	const int NPLANES = nvm_dev_attr_nplanes(dev);

	struct nvm_addr ppas[NPLANES];
	struct nvm_ioctl_dev_pio ctl;
	int i, ret;

	switch (type) {
		case 0x0:	/* MAGIC -- NVM_BLK_T_FREE aka "good" */
		case 0x1:	/* MAGIC -- NVM_BLK_T_BAD */
		case 0x2:	/* MAGIC -- NVM_BLK_T_GRWN_BAD */
			break;
		default:
			return -EINVAL;
	}

	for (i = 0; i < NPLANES; i++) {	/* Unroll over nplanes */
		struct nvm_addr ppa;

		ppa.ppa = addr.ppa;
		ppa.g.pg = 0;
		ppa.g.sec = 0;
		ppa.g.pl = i;

		ppas[i] = ppa;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = 0xf1;	/* MAGIC -- NVM_OP_PWRITE */
	ctl.flags = type;	/* MAGIC -- encoding bad-block type in flags */
	ctl.nppas = NPLANES;
	ctl.ppas = (uint64_t)ppas;

	ret = ioctl(dev->fd, NVM_DEV_PIO, &ctl);
	if (ret) {
		NVM_DEBUG("failed ret(%d)\n", ret);
		return ret;
	}

	if (ctl.result) {
		NVM_DEBUG("result(%u)\n", ctl.result);
		NVM_DEBUG("status(%llu)\n", (unsigned long long)ctl.status);
	}

	return ret;
}
