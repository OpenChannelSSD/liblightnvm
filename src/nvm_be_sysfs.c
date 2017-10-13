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
#ifndef NVM_BE_SYSFS_ENABLED
#include <liblightnvm.h>
#include <nvm_be.h>
struct nvm_be nvm_be_sysfs = {
	.id = NVM_BE_SYSFS,

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <nvm_dev.h>
#include <nvm_debug.h>
#include <nvm_utils.h>
#include <nvm_be.h>
#include <nvm_be_ioctl.h>

/**
 * Check that the given NVMe device exists in sysfs
 *
 * When nsid > 0 /sys/class/nvme/{nvme_name}/{nvme_name}n{nsid}
 * is checked for existance, /sys/class/nvme/{nvme_name} otherwise.
 *
 * @returns 1 when it exists 0 otherwise
 */
int sysfs_exists(const char *nvme_name, int nsid)
{
	const int path_buf_len = 0x1000;
	char path_buf[path_buf_len];

	DIR* dir;
	int ret;

	memset(path_buf, 0, sizeof(char) * path_buf_len);
	if (nsid) {
		sprintf(path_buf, "/sys/class/nvme/%s/%sn%d/lightnvm",
			nvme_name, nvme_name, nsid);
	} else {
		sprintf(path_buf, "/sys/class/nvme/%s", nvme_name);
	}

	dir = opendir(path_buf);
	ret = dir != NULL;

	if (dir)
		closedir(dir);

	return ret;
}

/**
 * Read sysfs attributes from NVMe devices.
 *
 * When nsid > 0 /sys/class/nvme/{nvme_name}/{nvme_name}n{nsid}/lightnvm/{attr}
 * is read  /sys/class/nvme/{nvme_name}/{attr} otherwise.
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int sysfs_to_buf(const char *nvme_name, int nsid, const char *attr,
			char *buf, int buf_len)
{
	const int path_buf_len = 0x1000;
	char path_buf[path_buf_len];
	FILE *fp;
	char c;

	memset(path_buf, 0, sizeof(char) * path_buf_len);
	if (nsid) {
		sprintf(path_buf, "/sys/class/nvme/%s/%sn%d/lightnvm/%s",
			nvme_name, nvme_name, nsid, attr);
	} else {
		sprintf(path_buf, "/sys/class/nvme/%s/%s", nvme_name, attr);
	}

	fp = fopen(path_buf, "rb");
	if (!fp)
		return -1;	// Propagate errno

	memset(buf, 0, sizeof(char) * buf_len);
	for (int i = 0; (((c = getc(fp)) != EOF) && i < buf_len); ++i)
		buf[i] = c;

	fclose(fp);

	return 0;
}

static int buf_to_fmt(const char *buf, struct nvm_spec_ppaf_nand *fmt,
		      struct nvm_spec_ppaf_nand_mask *mask)
{
	if (strlen(buf) != 27) { // len !matching "0x380830082808001010102008\n"
		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < 12; ++i) {
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

static long long int buf_to_ll(const char *buf)
{
	int base = 10;

	if ((strlen(buf) > 2) && (buf[0] == '0') && (buf[1] == 'x'))
		base = 16;

	return strtoll(buf, NULL, base);
}

static int populate(struct nvm_dev *dev)
{
	const int buf_len = 0x1000;
	char buf[buf_len];
	char nvme_name[buf_len];
	int nsid;

	struct nvm_geo *geo = &dev->geo;

	memset(nvme_name, 0, sizeof(char) * buf_len);
	if (nvm_be_split_dpath(dev->path, nvme_name, &nsid)) {
		NVM_DEBUG("FAILED: splitting dev_name: %s", dev->name);
		errno = EINVAL;
		return -1;
	}

	if (!sysfs_exists(nvme_name, nsid)) {
		NVM_DEBUG("FAILED: sysfs_exists");
		errno = ENODEV;
		return -1;
	}

	// Parse ppa_format from sysfs attribute
	if (sysfs_to_buf(nvme_name, nsid, "ppa_format", buf, buf_len)) {
		NVM_DEBUG("FAILED: reading ppa_format");
		errno = EINVAL;
		return -1;
	}

	if (buf_to_fmt(buf, &dev->ppaf, &dev->mask)) {
		NVM_DEBUG("FAILED: converting ppa_format");
		errno = EINVAL;
		return -1;
	}

	// Parse geometry from sysfs attributes
	if (sysfs_to_buf(nvme_name, nsid, "num_channels", buf,
				buf_len)) {
		NVM_DEBUG("FAILED: reading num_channels");
		errno = EIO;
		return -1;
	}
	geo->nchannels = buf_to_ll(buf);

	if (sysfs_to_buf(nvme_name, nsid, "num_luns", buf, buf_len)) {
		NVM_DEBUG("FAILED: reading num_luns");
		errno = EIO;
		return -1;
	}
	geo->nluns = buf_to_ll(buf);

	if (sysfs_to_buf(nvme_name, nsid, "num_planes", buf, buf_len)) {
		NVM_DEBUG("FAILED: reading num_planes");
		errno = EIO;
		return -1;
	}
	geo->nplanes = buf_to_ll(buf);

	if (sysfs_to_buf(nvme_name, nsid, "num_blocks", buf, buf_len)) {
		NVM_DEBUG("FAILED: reading num_blocks");
		errno = EIO;
		return -1;
	}
	geo->nblocks = buf_to_ll(buf);

	if (sysfs_to_buf(nvme_name, nsid, "num_pages", buf, buf_len)) {
		NVM_DEBUG("FAILED: reading num_pages");
		errno = EIO;
		return -1;
	}
	geo->npages = buf_to_ll(buf);

	if (sysfs_to_buf(nvme_name, nsid, "page_size", buf, buf_len)) {
		NVM_DEBUG("FAILED: reading page_size");
		errno = EIO;
		return -1;
	}
	geo->page_nbytes = buf_to_ll(buf);

	if (sysfs_to_buf(nvme_name, nsid, "hw_sector_size", buf,
				buf_len)) {
		NVM_DEBUG("FAILED: reading hw_sector_size");
		errno = EIO;
		return -1;
	}
	geo->sector_nbytes = buf_to_ll(buf);

	if (sysfs_to_buf(nvme_name, nsid, "oob_sector_size", buf,
				buf_len)) {
		NVM_DEBUG("FAILED: reading oob_sector_size");
		errno = EIO;
		return -1;
	}
	geo->meta_nbytes = buf_to_ll(buf);

	// Parse version identifier from sysfs attribute
	if (sysfs_to_buf(nvme_name, nsid, "version", buf, buf_len)) {
		NVM_DEBUG("FAILED: reading version");
		errno = EIO;
		return -1;
	}
	dev->verid = buf_to_ll(buf);

	// Parse media capabilities from sysfs attribute
	if (sysfs_to_buf(nvme_name, nsid, "media_capabilities", buf,
				buf_len)) {
		NVM_DEBUG("FAILED: reading media_capabilities");
		errno = EIO;
		return -1;
	}
	switch(dev->verid) {
	case NVM_SPEC_VERID_12:
		dev->mccap = buf_to_ll(buf);
		break;

	case NVM_SPEC_VERID_20:
		// The mccap exposed in sysfs on a 2.0 device is 1.2 fmt
		// it is weird, but how it is. So we shift it back.
		dev->mccap = (buf_to_ll(buf) & NVM_SPEC_12_MCCAP_SUSPENSION) >> 1;
		dev->mccap |= (buf_to_ll(buf) & NVM_SPEC_20_MCCAP_VCOPY) ? NVM_SPEC_20_MCCAP_VCOPY : 0;
		break;

	default:
		NVM_DEBUG("BAD dev->verid: %d", dev->verid);
		errno = EIO;
		return -1;
	}

	return 0;
}

void nvm_be_sysfs_close(struct nvm_dev *dev)
{
	close(dev->fd);
}

struct nvm_dev *nvm_be_sysfs_open(const char *dev_path, int flags)
{
	struct nvm_dev *dev;
	char nvme_name[NVM_DEV_PATH_LEN];
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

	// HACK: use naming conventions to determine nsid, fallback to hardcode
	if (nvm_be_split_dpath(dev_path, nvme_name, &dev->nsid))
		dev->nsid = 1;

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

	err = populate(dev);
	if (err) {
		NVM_DEBUG("FAILED: populate");
		close(dev->fd);
		free(dev);
		return NULL;
	}

	err = nvm_be_populate_derived(dev);
	if (err) {
		NVM_DEBUG("FAILED: nvm_be_populate_derived");
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

	.idfy = nvm_be_ioctl_idfy,
	.rprt = nvm_be_ioctl_rprt,
	.sbbt = nvm_be_ioctl_sbbt,
	.gbbt = nvm_be_ioctl_gbbt,

	.erase = nvm_be_ioctl_erase,
	.write = nvm_be_ioctl_write,
	.read = nvm_be_ioctl_read,
	.copy = nvm_be_ioctl_copy,
};
#endif

