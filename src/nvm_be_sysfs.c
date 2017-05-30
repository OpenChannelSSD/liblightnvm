/*
 * be_sysfs - Backend based on kernel support for NVM IOCTL and sysfs
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <libudev.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_be_ioctl.h>
#include <nvm_dev.h>
#include <nvm_debug.h>
#include <nvm_utils.h>

static inline uint64_t _ilog2(uint64_t x)
{
	uint64_t val = 0;

	while (x >>= 1)
		val++;

	return val;
}

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
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;

	enumerate = udev_enumerate_new(udev);	/* Search 'subsystem' */
	if (!enumerate) {
		errno = ENOMEM;
		return NULL;
	}

	udev_enumerate_add_match_subsystem(enumerate, subsystem);
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);
	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path = udev_list_entry_get_name(dev_list_entry);

		if (!path) {
			NVM_DEBUG("FAILED: retrieving path from entry\n");
			continue;
		}

		int path_len = strlen(path);

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
			NVM_DEBUG("FAILED: retrieving device from path\n");
			continue;
		}

		if (devtype) {			/* Compare device type */
			const char *sys_devtype = udev_device_get_devtype(dev);
			if (!sys_devtype) {
				NVM_DEBUG("FAILED: sys_devtype(%s)", sys_devtype);
				udev_device_unref(dev);
				dev = NULL;
				continue;
			}

			int sys_devtype_match = strcmp(devtype, sys_devtype);
			if (sys_devtype_match != 0) {
				NVM_DEBUG("FAILED: %s != %s\n", devtype, sys_devtype);
				udev_device_unref(dev);
				dev = NULL;
				continue;
			}
		}

		break;
	}

	udev_enumerate_unref(enumerate);

	return dev;
}

struct udev_device *udev_nvmdev_find(struct udev *udev, const char *dev_name)
{
	struct udev_device *dev;

	dev  = udev_dev_find(udev, "block", NULL, dev_name);
	if (dev)
		return dev;

	NVM_DEBUG("FAILED: NOTHING FOUND\n");
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

	if ((strlen(buf) > 2) && (buf[0] == '0') && (buf[1] == 'x')) {
		*val = strtol(buf, NULL, 16);
	} else {
		*val = atoi(buf);
	}

	return 0;
}

static int sysattr2fmt(struct udev_device *dev, const char *attr,
		       struct nvm_spec_ppaf_nand  *fmt, struct nvm_spec_ppaf_nand_mask *mask)
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

	if (strlen(buf) != 27) { // len !matching "0x380830082808001010102008\n"
		return -1;
	}

	for (i = 0; i < 12; ++i) {
		char buf_fmt[3];

		buf_fmt[0] = buf[2 + i*2];	// offset in bits
		buf_fmt[1] = buf[2 + i*2 + 1];	// number of bits
		buf_fmt[2] = '\0';
		fmt->a[i] = strtol(buf_fmt, NULL, 16);

		if ((i % 2)) {
			// i-1 = offset
			// i = width
			mask->a[i/2] = (((uint64_t)1<< fmt->a[i])-1) << fmt->a[i-1];
		}
	}

	return 0;
}

static int dev_attr_fill(struct nvm_dev *dev)
{
	struct udev *udev;
	struct udev_device *udev_dev;
	struct nvm_geo *geo;
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
	if (sysattr2fmt(udev_dev, "lightnvm/ppa_format", &dev->ppaf, &dev->mask)) {
		NVM_DEBUG("FAILED: ppa_format for name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}

	/*
	 * Extract geometry from sysfs via libudev
	 */
	geo = &(dev->geo);

	if (sysattr2int(udev_dev, "lightnvm/num_channels", &val)) {
		NVM_DEBUG("FAILED: num_channels for dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->nchannels = val;

	if (sysattr2int(udev_dev, "lightnvm/num_luns", &val)) {
		NVM_DEBUG("FAILED: num_luns for dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->nluns = val;

	if (sysattr2int(udev_dev, "lightnvm/num_planes", &val)) {
		NVM_DEBUG("FAILED: num_planes for dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->nplanes = val;

	if (sysattr2int(udev_dev, "lightnvm/num_blocks", &val)) {
		NVM_DEBUG("FAILED: num_blocks for dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->nblocks = val;

	if (sysattr2int(udev_dev, "lightnvm/num_pages", &val)) {
		NVM_DEBUG("FAILED: num_pages for dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->npages = val;

	if (sysattr2int(udev_dev, "lightnvm/page_size", &val)) {
		NVM_DEBUG("FAILED: page_size for dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->page_nbytes = val;

	if (sysattr2int(udev_dev, "lightnvm/hw_sector_size", &val)) {
		NVM_DEBUG("FAILED: hw_sector_size for dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->sector_nbytes = val;

	if (sysattr2int(udev_dev, "lightnvm/oob_sector_size", &val)) {
		NVM_DEBUG("FAILED: oob_sector_size dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	geo->meta_nbytes = val;

	if (sysattr2int(udev_dev, "lightnvm/version", &val)) {
		NVM_DEBUG("FAILED: version dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	dev->verid = val;

	if (sysattr2int(udev_dev, "lightnvm/media_capabilities", &val)) {
		NVM_DEBUG("FAILED: capabilities dev->name(%s)\n", dev->name);
		errno = EIO;
		goto exit;
	}
	switch(dev->verid) {
	case NVM_SPEC_VERID_12:
		dev->mccap = val;
		break;
	case NVM_SPEC_VERID_20:
		// The mccap exposed in sysfs on a 2.0 device is 1.2 fmt
		// it is weird, but how it is. So we shift it back.
		dev->mccap = (val & NVM_SPEC_12_MCCAP_SUSPENSION) >> 1;
		dev->mccap |= (val & NVM_SPEC_20_MCCAP_VCOPY) ? NVM_SPEC_20_MCCAP_VCOPY : 0;
		break;
	default:
		NVM_DEBUG("BAD dev->verid: %d", dev->verid);
		errno = EIO;
		goto exit;
	}

	// WARN: HOTFIX for reports of unrealisticly large OOB area
	if (geo->meta_nbytes > (dev->geo.sector_nbytes * 0.1)) {
		geo->meta_nbytes = 16;	// Naively hope this is right
	}

	// Derive number of sectors
	geo->nsectors = geo->page_nbytes / geo->sector_nbytes;

	/* Derive total number of bytes on device */
	geo->tbytes = geo->nchannels * geo->nluns * geo->nplanes * \
		      geo->nblocks * geo->npages * geo->nsectors * geo->sector_nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	dev->ssw = _ilog2(geo->sector_nbytes);

	/* Derive a default plane mode */
	switch(geo->nplanes) {
	case 4:
		dev->pmode = NVM_FLAG_PMODE_QUAD;
		break;
	case 2:
		dev->pmode = NVM_FLAG_PMODE_DUAL;
		break;
	case 1:
		dev->pmode = NVM_FLAG_PMODE_SNGL;
		break;
	default:
		NVM_DEBUG("FAILED: invalid geo->nplanes: %lu", geo->nplanes);
		errno = EINVAL;
		goto exit;
	}

	dev->erase_naddrs_max = NVM_NADDR_MAX;
	dev->write_naddrs_max = NVM_NADDR_MAX;
	dev->read_naddrs_max = NVM_NADDR_MAX;

	dev->meta_mode = NVM_META_MODE_NONE;

	errno = 0;
exit:
	udev_device_unref(udev_dev);
	udev_unref(udev);

	return errno ? -1 : 0;
}

void nvm_be_sysfs_close(struct nvm_dev *dev)
{
	close(dev->fd);
}

struct nvm_dev *nvm_be_sysfs_open(const char *dev_path, int NVM_UNUSED(flags))
{
	struct nvm_dev *dev;
	int err;
	
	if (strlen(dev_path) > NVM_DEV_PATH_LEN) {
		NVM_DEBUG("FAILED: Device path too long\n");
		errno = EINVAL;
		return NULL;
	}

	dev = malloc(sizeof(*dev));
	if (!dev) {
		NVM_DEBUG("FAILED: allocating 'struct nvm_dev'\n");
		return NULL;	// Propagate errno from malloc
	}

	strncpy(dev->path, dev_path, NVM_DEV_PATH_LEN);
	strncpy(dev->name, dev_path+5, NVM_DEV_NAME_LEN);

	dev->fd = open(dev->path, O_RDONLY);
	if (dev->fd < 0) {
		NVM_DEBUG("FAILED: open dev->path:%s, dev->fd: %d, errno: %d\n",
			  dev->path, dev->fd, errno);

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

struct nvm_be nvm_be_sysfs = {
	.id = NVM_BE_SYSFS,

	.open = nvm_be_sysfs_open,
	.close = nvm_be_sysfs_close,

	.user = nvm_be_ioctl_user,
	.admin = nvm_be_ioctl_admin,

	.vuser = nvm_be_ioctl_vuser,
	.vadmin = nvm_be_ioctl_vadmin
};

