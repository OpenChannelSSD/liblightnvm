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
#include <libudev.h>
#include <stdio.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

static struct nvm_dev *devices = NULL;

struct nvm_dev_info* nvm_dev_info_new(void)
{
	struct nvm_dev_info *info = malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	return info;
}

void nvm_dev_info_free(struct nvm_dev_info **info)
{
	if (!info || !*info)
		return;

	free(*info);
	*info = NULL;
}

void nvm_dev_info_pr(struct nvm_dev_info *info)
{
	printf("dev_info {\n"
		"  dev_name(%s),\n"
		"  version(%u),\n"
		"  vendor_opcode(%u),\n"
		"  capabilities(%u),\n"
		"  device_mode(%u),\n"
		"  media_manager(%s),\n"
		"  ppaf(\n"
		"    ch_offset(%u, 0x%02x),\n"
		"    ch_len(%u, 0x%02x),\n"
		"    lun_offset(%u, 0x%02x),\n"
		"    lun_len(%u, 0x%02x),\n"
		"    pln_offset(%u, 0x%02x),\n"
		"    pln_len(%u, 0x%02x),\n"
		"    blk_offset(%u, 0x%02x),\n"
		"    blk_len(%u, 0x%02x),\n"
		"    pg_offset(%u, 0x%02x),\n"
		"    pg_len(%u, 0x%02x),\n"
		"    sect_offset(%u, 0x%02x),\n"
		"    sect_len(%u, 0x%02x)\n"
		"  ),\n"
		"  fpg_size(%d)\n"
		"  sec_size(%d),\n"
		"  sec_per_pg(%d),\n"
		"  media_type(%u),\n"
		"  flash_media_type(%u),\n"
		"  num_channels(%u),\n"
		"  num_luns(%u),\n"
		"  num_planes(%u),\n"
		"  num_blocks(%u),\n"
		"  page_size(%u),\n"
		"  hw_sector_size(%u),\n"
		"  oob_sector_size(%u),\n"
		"  read_typ(%u),\n"
		"  read_max(%u),\n"
		"  prog_typ(%u),\n"
		"  prog_max(%u),\n"
		"  erase_typ(%u),\n"
		"  erase_max(%u),\n"
		"  multiplane_modes(%u, 0x%08x),\n"
		"  media_capabilities(%u, 0x%08x),\n"
		"  channel_parallelism(%u)\n"
		"  max_phys_sect(%u)\n"
		"}\n",
		info->dev_name,
		info->version,
		info->vendor_opcode,
		info->capabilities,
		info->device_mode,
		info->media_manager,

		info->ch_offset,
		info->ch_offset,
		info->ch_len,
		info->ch_len,
		info->lun_offset,
		info->lun_offset,
		info->lun_len,
		info->lun_len,
		info->pln_offset,
		info->pln_offset,
		info->pln_len,
		info->pln_len,
		info->blk_offset,
		info->blk_offset,
		info->blk_len,
		info->blk_len,
		info->pg_offset,
		info->pg_offset,
		info->pg_len,
		info->pg_len,
		info->sect_offset,
		info->sect_offset,
		info->sect_len,
		info->sect_len,

		info->fpg_size,
		info->sec_size,
		info->sec_per_pg,

		info->media_type,
		info->flash_media_type,
		info->num_channels,
		info->num_luns,
		info->num_planes,
		info->num_blocks,
		info->page_size,
		info->hw_sector_size,
		info->oob_sector_size,
		info->read_typ,
		info->read_max,
		info->prog_typ,
		info->prog_max,
		info->erase_typ,
		info->erase_max,

		info->multiplane_modes,
		info->multiplane_modes,
		info->media_capabilities,
		info->media_capabilities,
		info->channel_parallelism,
		info->max_phys_sect
		);
}

int nvm_dev_info_fill(struct nvm_dev_info *info, const char *dev_name)
{
	/* libudev implementation */
	struct udev *udev;
	struct udev_device *dev;

	udev = udev_new();
	if (!udev) {
		printf("dev_info_fill: Failed creating udev.\n");
		return -1;
	}

	dev = udev_dev_find(udev, "nvme", "lightnvm", dev_name);
	if (!dev) {
		printf("CANNOT FIND DEVICE\n");
		udev_unref(udev);
		return -1;
	}

	strncpy(info->dev_name, dev_name, DISK_NAME_LEN);
	info->version = atoi(udev_device_get_sysattr_value(dev, "lightnvm/version"));
	info->vendor_opcode = atoi(udev_device_get_sysattr_value(dev, "lightnvm/vendor_opcode"));
	info->capabilities = atoi(udev_device_get_sysattr_value(dev, "lightnvm/capabilities"));
	info->device_mode = atoi(udev_device_get_sysattr_value(dev, "lightnvm/device_mode"));
	strncpy(info->media_manager, udev_device_get_sysattr_value(dev, "lightnvm/media_manager"), NVM_TTYPE_NAME_MAX);

	const char *ppaf_hex = udev_device_get_sysattr_value(dev, "lightnvm/ppa_format");
	char buf[3] = { 0 };

	memcpy(buf, ppaf_hex+2, 2);
	info->ch_offset = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+4, 2);
	info->ch_len = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+6, 2);
	info->lun_offset = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+8, 2);
	info->lun_len = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+10, 2);
	info->pln_offset = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+12, 2);
	info->pln_len = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+14, 2);
	info->blk_offset = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+16, 2);
	info->blk_len = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+18, 2);
	info->pg_offset = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+20, 2);
	info->pg_len = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+22, 2);
	info->sect_offset = strtol(buf, NULL, 16);
	memcpy(buf, ppaf_hex+24, 2);
	info->sect_len = strtol(buf, NULL, 16);

	info->fpg_size = atoi(udev_device_get_sysattr_value(dev, "lightnvm/fpg_size"));
	info->sec_size = atoi(udev_device_get_sysattr_value(dev, "lightnvm/sec_size"));
	info->sec_per_pg = atoi(udev_device_get_sysattr_value(dev, "lightnvm/sec_per_pg"));
	
	info->media_type = atoi(udev_device_get_sysattr_value(dev, "lightnvm/media_type"));
	info->flash_media_type = atoi(udev_device_get_sysattr_value(dev, "lightnvm/flash_media_type"));
	info->num_channels = atoi(udev_device_get_sysattr_value(dev, "lightnvm/num_channels"));
	info->num_luns = atoi(udev_device_get_sysattr_value(dev, "lightnvm/num_luns"));
	info->num_planes = atoi(udev_device_get_sysattr_value(dev, "lightnvm/num_planes"));

	info->num_blocks = atoi(udev_device_get_sysattr_value(dev, "lightnvm/num_blocks"));
	info->num_pages = atoi(udev_device_get_sysattr_value(dev, "lightnvm/num_pages"));
	info->page_size = atoi(udev_device_get_sysattr_value(dev, "lightnvm/page_size"));
	info->hw_sector_size = atoi(udev_device_get_sysattr_value(dev, "lightnvm/hw_sector_size"));
	info->oob_sector_size = atoi(udev_device_get_sysattr_value(dev, "lightnvm/oob_sector_size"));

	info->read_typ = atoi(udev_device_get_sysattr_value(dev, "lightnvm/read_typ"));
	info->read_max = atoi(udev_device_get_sysattr_value(dev, "lightnvm/read_max"));
	info->prog_typ = atoi(udev_device_get_sysattr_value(dev, "lightnvm/prog_typ"));
	info->prog_max = atoi(udev_device_get_sysattr_value(dev, "lightnvm/prog_max"));
	info->erase_typ = atoi(udev_device_get_sysattr_value(dev, "lightnvm/erase_typ"));
	info->erase_max = atoi(udev_device_get_sysattr_value(dev, "lightnvm/erase_max"));

	info->multiplane_modes = strtol(udev_device_get_sysattr_value(dev, "lightnvm/multiplane_modes"), NULL, 0);
	info->media_capabilities = strtol(udev_device_get_sysattr_value(dev, "lightnvm/media_capabilities"), NULL, 0);
	info->channel_parallelism = atoi(udev_device_get_sysattr_value(dev, "lightnvm/channel_parallelism"));
	
	info->max_phys_sect = atoi(udev_device_get_sysattr_value(dev, "lightnvm/max_phys_sect"));

	udev_device_unref(dev);
	udev_unref(udev);

	/* Calculate magic value */
	info->pln_pg_size = info->fpg_size * info->num_planes;

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
	NVM_DEBUG("called\n");
	if (!dev)
		return;

	NVM_DEBUG("Freeing\n");
	free(*dev);
	*dev = NULL;
}

void nvm_dev_pr(struct nvm_dev* dev)
{
	printf("nvm_dev {}\n");
}

struct nvm_dev_info* nvm_dev_get_info(struct nvm_dev *dev)
{
	return &dev->info;
}

int nvm_dev_get_pln_pg_size(struct nvm_dev *dev)
{
	return dev->info.pln_pg_size;
}

int nvm_dev_get_sec_size(struct nvm_dev *dev)
{
	return dev->info.hw_sector_size;
}

struct nvm_dev* nvm_dev_open(const char *dev_name)
{
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
	
	err = nvm_dev_info_fill(&dev->info, dev_name);
	if (err) {
		NVM_DEBUG("nvm_dev_open: _fill failed(%d)\n", err);
		nvm_dev_free(&dev);
		return NULL;
	}
	
	atomic_inc(&dev->ref_cnt);
	HASH_ADD_STR(devices, info.dev_name, dev);

	return dev;
}

void nvm_dev_close(struct nvm_dev *dev)
{
	NVM_DEBUG("called\n");
	if (atomic_dec_and_test(&(dev)->ref_cnt)) {
		HASH_DEL(devices, dev);
		nvm_dev_free(&dev);
	}
}
