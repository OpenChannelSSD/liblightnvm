/*
 * dev - Device functions
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <libudev.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

/*
 * Searches the udev 'subsystem' for device named 'dev_name' of type 'devtype'
 *
 * NOTE: Caller is responsible for calling `udev_device_unref` on the returned
 * udev_device
 *
 * @returns First device in 'subsystem' of given 'devtype' with given 'dev_name'
 */
struct udev_device *udev_dev_find(struct udev *udev, const char *subsystem,
				  const char *devtype, const char *dev_name)
{
	struct udev_device *dev = NULL;

	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;

	enumerate = udev_enumerate_new(udev);	/* Search 'subsystem' */
	udev_enumerate_add_match_subsystem(enumerate, subsystem);
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path;
		int path_len;

		path = udev_list_entry_get_name(dev_list_entry);
		if (!path) {
			NVM_DEBUG("Failed retrieving path from entry\n");
			continue;
		}
		path_len = strlen(path);

		if (dev_name) {			/* Compare name */
			int dev_name_len = strlen(dev_name);
			int match = strcmp(dev_name,
					   path + path_len-dev_name_len);
			if (match != 0) {
				continue;
			}
		}
						/* Get the udev object */
		dev = udev_device_new_from_syspath(udev, path);
		if (!dev) {
			NVM_DEBUG("Failed retrieving device from path\n");
			continue;
		}

		if (devtype) {			/* Compare device type */
			const char *sys_devtype;
			int sys_devtype_match;

			sys_devtype = udev_device_get_devtype(dev);
			if (!sys_devtype) {
				NVM_DEBUG("sys_devtype(%s)", sys_devtype);
				udev_device_unref(dev);
				dev = NULL;
				continue;
			}

			sys_devtype_match = strcmp(devtype, sys_devtype);
			if (sys_devtype_match != 0) {
				NVM_DEBUG("%s != %s\n", devtype, sys_devtype);
				udev_device_unref(dev);
				dev = NULL;
				continue;
			}
		}

		break;
	}

	return dev;
}

struct udev_device *udev_nvmdev_find(struct udev *udev, const char *dev_name)
{
	struct udev_device *dev;

	dev  = udev_dev_find(udev, "block", NULL, dev_name);
	if (dev)
		return dev;

	NVM_DEBUG("NOTHING FOUND\n");
	return NULL;
}

static int sysattr2int(struct udev_device *dev, const char *attr, int *val)
{
	const char *dev_path;
	char path[4096];
	char buf[4096];
	char c;
	FILE *fp;
	int i;

	memset(buf, 0, sizeof(char)*4096);

	dev_path = udev_device_get_syspath(dev);
	if (!dev_path)
		return -ENODEV;

	sprintf(path, "%s/%s", dev_path, attr);
	fp = fopen(path, "rb");
	if (!fp)
		return -ENODEV;

	i = 0;
	while (((c = getc(fp)) != EOF) && i < 4096) {
		buf[i] = c;
		++i;
	}
	fclose(fp);

	*val = atoi(buf);
	return 0;
}

static int sysattr2fmt(struct udev_device *dev, const char *attr,
		   struct nvm_addr_fmt  *fmt)
{
	const char *dev_path;
	char path[4096];
	char buf[4096];
	char buf_fmt[3];
	char c;
	FILE *fp;
	int i;

	memset(buf, 0, sizeof(char)*4096);

	dev_path = udev_device_get_syspath(dev);
	if (!dev_path)
		return -ENODEV;

	sprintf(path, "%s/%s", dev_path, attr);
	fp = fopen(path, "rb");
	if (!fp)
		return -ENODEV;

	i = 0;
	while (((c = getc(fp)) != EOF) && i < 4096) {
		buf[i] = c;
		++i;
	}
	fclose(fp);

	if (strlen(buf) != 27) { // len !matching "0x380830082808001010102008\n"
		return -1;
	}

	for (i = 0; i < 12; ++i) {
		buf_fmt[0] = buf[2 + i*2];
		buf_fmt[1] = buf[2 + i*2 + 1];
		buf_fmt[2] = '\0';
		fmt->a[i] = strtol(buf_fmt, NULL, 16);
	}

	return 0;
}

static int dev_attr_fill(struct nvm_dev *dev)
{
	struct udev *udev;
	struct udev_device *udev_dev;
	int val;

	udev = udev_new();
	if (!udev) {
		NVM_DEBUG("FAILED: udev_new for name(%s)\n", dev->name);
		errno = ENOMEM;
		return -1;
	}

	/* Get a handle on udev / sysfs */
	udev_dev = udev_nvmdev_find(udev, dev->name);
	if (!udev_dev) {
		NVM_DEBUG("FAILED: udev_nvmdev_find for name(%s)\n", dev->name);
		udev_unref(udev);
		errno = ENODEV;
		return -1;
	}

	/* Extract ppa_format from sysfs via libudev */
	if (sysattr2fmt(udev_dev, "lightnvm/ppa_format", &dev->fmt)) {
		NVM_DEBUG("FAILED: ppa_format for name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}

	/*
	 * Extract geometry from sysfs via libudev
	 */

	if (sysattr2int(udev_dev, "lightnvm/num_channels", &val)) {
		NVM_DEBUG("ERR: num_channels for dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.nchannels = val;

	if (sysattr2int(udev_dev, "lightnvm/num_luns", &val)) {
		NVM_DEBUG("ERR: num_luns for dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.nluns = val;

	if (sysattr2int(udev_dev, "lightnvm/num_planes", &val)) {
		NVM_DEBUG("ERR: num_planes for dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.nplanes = val;

	if (sysattr2int(udev_dev, "lightnvm/num_blocks", &val)) {
		NVM_DEBUG("ERR: num_blocks for dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.nblocks = val;

	if (sysattr2int(udev_dev, "lightnvm/num_pages", &val)) {
		NVM_DEBUG("ERR: num_pages for dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.npages = val;

	if (sysattr2int(udev_dev, "lightnvm/sec_per_pg", &val)) {
		NVM_DEBUG("ERR: sec_per_pg for dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.nsectors = val;

	if (sysattr2int(udev_dev, "lightnvm/hw_sector_size", &val)) {
		NVM_DEBUG("ERR: hw_sector_size for dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.nbytes = val;

	if (sysattr2int(udev_dev, "lightnvm/oob_sector_size", &val)) {
		NVM_DEBUG("ERR: oob_sector_size dev->name(%s)\n", dev->name);
		errno = EIO;
		return -1;
	}
	dev->geo.meta_nbytes = val;

	// WARN: HOTFIX for reports of unrealisticly large oob area
	if (dev->geo.meta_nbytes > 100) {
		dev->geo.meta_nbytes = 16;	// Naively hope this is right
	}

	/* Derive total number of bytes on device */
	dev->geo.tbytes = dev->geo.nchannels * dev->geo.nluns * \
			  dev->geo.nplanes * dev->geo.nblocks * \
			  dev->geo.npages * dev->geo.nsectors * dev->geo.nbytes;

	/* Derive number of bytes occupied by a virtual block/page */
	dev->geo.vblk_nbytes = dev->geo.nplanes * dev->geo.npages * \
				dev->geo.nsectors * dev->geo.nbytes;
	dev->geo.vpg_nbytes = dev->geo.nplanes * dev->geo.nsectors * \
				dev->geo.nbytes;

	udev_device_unref(udev_dev);
	udev_unref(udev);

	return 0;
}

struct nvm_dev *nvm_dev_new(void)
{
	struct nvm_dev *dev;

	dev = malloc(sizeof(*dev));
	if (dev)
		memset(dev, 0, sizeof(*dev));

	return dev;
}

void nvm_dev_pr(struct nvm_dev *dev)
{
	printf("dev { path(%s), name(%s), fd(%d) }\n",
	       dev->path, dev->name, dev->fd);
	printf("dev-"); nvm_geo_pr(dev->geo);
	printf("dev-"); nvm_addr_fmt_pr(&dev->fmt);
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

struct nvm_dev *nvm_dev_open(const char *dev_path)
{
	struct nvm_dev *dev;
	int err;
	
	if (strlen(dev_path) > NVM_DEV_PATH_LEN) {
		NVM_DEBUG("FAILED: Device path too long\n");
		errno = EINVAL;
		return NULL;
	}

	dev = nvm_dev_new();
	if (!dev) {
		NVM_DEBUG("FAILED: nvm_dev_new.\n");
		return NULL;
	}

	strncpy(dev->path, dev_path, NVM_DEV_PATH_LEN);
	strncpy(dev->name, dev_path+5, NVM_DEV_NAME_LEN);

	dev->fd = open(dev->path, O_RDWR);
	if (dev->fd < 0) {
		NVM_DEBUG("FAILED: open dev->path(%s) dev->fd(%d)\n",
			  dev->path, dev->fd);

		free(dev);

		return NULL;
	}

	err = dev_attr_fill(dev);
	if (err) {
		NVM_DEBUG("FAILED: dev_attr_fill, err(%d)\n", err);
		close(dev->fd);
		free(dev);
		return NULL;
	}

	return dev;
}

void nvm_dev_close(struct nvm_dev *dev)
{
	if (dev && !dev->fd)
		close(dev->fd);
	free(dev);
}

