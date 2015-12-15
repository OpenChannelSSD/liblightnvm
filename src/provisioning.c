/*
 * provisioning - LightNVM get/put block API
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
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
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <liblightnvm.h>

#include "provisioning.h"
#include "../util/debug.h"

static struct nvm_device *devt = NULL;
static struct nvm_target *tgtt = NULL;
static struct nvm_target_map *tgtmt = NULL;

static void device_clean(struct nvm_device *dev)
{
	HASH_DEL(devt, dev);
	free(dev);
}

static void target_clean(struct nvm_target *tgt)
{
	struct nvm_device *dev = tgt->dev;

	HASH_DEL(tgtt, tgt);
	if (atomic_dec_and_test(&dev->ref_cnt))
		device_clean(dev);
	free(tgt);
}

static void target_ins_clean(int tgt)
{
	struct nvm_target *t;
	struct nvm_target_map *tm;

	HASH_FIND_INT(tgtmt, &tgt, tm);
	if (tm) {
		t = tm->tgt;
		if (atomic_dec_and_test(&t->ref_cnt))
			target_clean(t);
		HASH_DEL(tgtmt, tm);
		free(tm);
	}
}

static inline int get_dev_info(char *dev, struct nvm_device *device)
{
	struct nvm_ioctl_dev_info *dev_info = &device->info;
	struct nvm_fpage *fpage = &device->flash_page;
	int ret = 0;
	int i;

	/* Initialize ioctl values */
	memset(dev_info->bmname, 0, NVM_TTYPE_MAX);
	for (i = 0; i < 3; i++)
		dev_info->bmversion[i] = 0;
	for (i = 0; i < 8; i++)
		dev_info->reserved[i] = 0;
	dev_info->flags = 0;
	dev_info->prop.sec_size = 0;
	dev_info->prop.sec_per_page = 0;
	dev_info->prop.max_sec_io = 0;
	dev_info->prop.nr_planes = 0;
	dev_info->prop.nr_luns = 0;
	dev_info->prop.nr_channels = 0;
	dev_info->prop.plane_mode = 0;
	dev_info->prop.oob_size = 0;

	strncpy(dev_info->dev, dev, DISK_NAME_LEN);
	ret = nvm_get_device_info(dev_info);
	if (ret)
		goto out;

	/* Calculated values */
	fpage->sec_size = dev_info->prop.sec_size;
	fpage->page_size = fpage->sec_size * dev_info->prop.sec_per_page;
	fpage->pln_pg_size = fpage->page_size * dev_info->prop.nr_planes;
	fpage->max_sec_io = dev_info->prop.max_sec_io;

	LNVM_DEBUG("Device cached: %s(sizes:%d/%d/%d, max_sec_io:%d)\n",
			device->info.dev,
			device->flash_page.sec_size,
			device->flash_page.page_size,
			device->flash_page.pln_pg_size,
			device->info.prop.max_sec_io);
out:
	return ret;
}

static inline int get_tgt_info(char *tgt, char *dev,
						struct nvm_target *target)
{
	struct nvm_ioctl_tgt_info tgt_info;
	int ret = 0;

	/* Initialize ioctl values */
	tgt_info.version[0] = 0;
	tgt_info.version[1] = 0;
	tgt_info.version[2] = 0;
	tgt_info.reserved = 0;
	memset(tgt_info.target.dev, 0, DISK_NAME_LEN);
	memset(tgt_info.target.tgttype, 0, NVM_TTYPE_MAX);

	strncpy(tgt_info.target.tgtname, tgt, DISK_NAME_LEN);
	ret = nvm_get_target_info(&tgt_info);
	if (ret)
		goto out;

	target->version[0] = tgt_info.version[0];
	target->version[1] = tgt_info.version[1];
	target->version[2] = tgt_info.version[2];
	target->reserved = tgt_info.reserved;
	strncpy(target->tgtname, tgt_info.target.tgtname, DISK_NAME_LEN);
	strncpy(target->tgttype, tgt_info.target.tgttype, NVM_TTYPE_NAME_MAX);
	strncpy(dev, tgt_info.target.dev, DISK_NAME_LEN);

	LNVM_DEBUG("Target cached: %s(t:%s, d:%s)\t(%u,%u,%u)\n",
			target->tgtname, target->tgttype, dev,
			target->version[0], target->version[1],
			target->version[2]);
out:
	return ret;
}

struct nvm_target_map *get_nvm_tgt_map(int tgt){
	struct nvm_target_map *tgt_map;

	HASH_FIND_INT(tgtmt, &tgt, tgt_map);
	if (!tgt_map)
		return NULL;
	return tgt_map;
}

/*
 * Target management
 */

/*TODO: Add which application opens tgt? */
int nvm_target_open(const char *tgt, int flags)
{
	struct nvm_device *dev_entry;
	struct nvm_target *tgt_entry;
	struct nvm_target_map *tgtm_entry;
	char tgt_loc[NVM_TGT_NAME_MAX] = "/dev/";
	char tgt_eol[DISK_NAME_LEN];
	char dev[DISK_NAME_LEN];
	int tgt_id;
	int new_dev = 0, new_tgt = 0;
	int ret = 0;

	strcpy(tgt_eol, tgt);
	tgt_eol[DISK_NAME_LEN - 1] = '\0';

	HASH_FIND_STR(tgtt, tgt_eol, tgt_entry);
	if (!tgt_entry) {
		tgt_entry = malloc(sizeof(struct nvm_target));
		if (!tgt_entry)
			return -ENOMEM;

		ret = get_tgt_info(tgt_eol, dev, tgt_entry);
		if (ret) {
			LNVM_DEBUG("Could not get target information\n");
			free(tgt_entry);
			return ret;
		}

		dev[DISK_NAME_LEN - 1] = '\0';
		HASH_FIND_STR(devt, dev, dev_entry);
		if (!dev_entry) {
			dev_entry = malloc(sizeof(struct nvm_device));
			if (!dev_entry) {
				free(tgt_entry);
				return -ENOMEM;
			}

			ret = get_dev_info(dev, dev_entry);
			if (ret) {
				LNVM_DEBUG("Could not get device information\n");
				free(dev_entry);
				free(tgt_entry);
				return ret;
			}

			strncpy(dev_entry->info.dev, dev, DISK_NAME_LEN);
			HASH_ADD_STR(devt, info.dev, dev_entry);

			atomic_init(&dev_entry->ref_cnt);
			atomic_set(&dev_entry->ref_cnt, 0);
			new_dev = 1;
		}

		tgt_entry->dev = dev_entry;
		strncpy(tgt_entry->tgtname, tgt_eol, DISK_NAME_LEN);
		HASH_ADD_STR(tgtt, tgtname, tgt_entry);
		atomic_inc(&dev_entry->ref_cnt);

		atomic_init(&tgt_entry->ref_cnt);
		atomic_set(&tgt_entry->ref_cnt, 0);
		new_tgt = 1;
	}

	strcat(tgt_loc, tgt);
	tgt_id = open(tgt_loc, O_RDWR | O_DIRECT);
	if (tgt_id < 0) {
		LNVM_DEBUG("Failed to open LightNVM target %s (%d)\n",
				tgt_loc, tgt_id);
		goto error;
	}

	tgtm_entry = malloc(sizeof(struct nvm_target_map));
	if (!tgtm_entry) {
		ret = -ENOMEM;
		goto error;
	}

	tgtm_entry->tgt_id = tgt_id;
	tgtm_entry->tgt = tgt_entry;
	HASH_ADD_INT(tgtmt, tgt_id, tgtm_entry);
	atomic_inc(&tgt_entry->ref_cnt);

	return tgt_id;

error:
	if (new_dev)
		free(dev_entry);
	if (new_tgt)
		free(tgt_entry);

	return ret;
}

void nvm_target_close(int tgt)
{
	close(tgt);
	target_ins_clean(tgt);
}

/*
 * Provisioning
 */
int nvm_get_block(int tgt, uint32_t lun, NVM_PROV *prov)
{
	NVM_VBLOCK *vblock = prov->vblock;
	struct nvm_ioctl_lun_status lun_status = {
		.nr_free_blocks = 0,
		.nr_inuse_blocks = 0,
		.nr_bad_blocks = 0,
	};
	int ret = 0;

	/* Initialize ioctl values */
	vblock->id = 0;
	vblock->bppa = 0;
	vblock->nppas = 0;
	vblock->ppa_bitmap = 0;

	vblock->vlun_id = lun;
	vblock->owner_id = 101;
	vblock->flags = 0x0;

	prov->lun_status = &lun_status;

	ret = ioctl(tgt, NVM_PR_GET_BLOCK, prov);
	if (ret) {
		LNVM_DEBUG("Could not get block from lun %d\n", lun);
		goto out;
	}

	LNVM_DEBUG("Get block from lun %d. Block id:%llu bppa:%llu\n",
			vblock->vlun_id,
			vblock->id,
			vblock->bppa);
out:
	return ret;
}

#if 0
int nvm_get_block_meta(int tgt, uint64_t vblock_id, NVM_VBLOCK *vblock)
{
	struct nvm_ioctl_provisioning prov = {
		.flags = 0,
	};
	struct nvm_ioctl_lun_status lun_status = {
		.nr_free_blocks = 0,
		.nr_inuse_blocks = 0,
		.nr_bad_blocks = 0,
	};
	int ret = 0;

#ifdef LNVM_DEBUG_ENABLED
	/* Initialize all bytes in structure, including padding for debugging */
	memset(&prov, 0, sizeof(prov));
#endif

	/* Initialize ioctl values */
	vblock->bppa = 0;
	vblock->nppas = 0;
	vblock->ppa_bitmap = 0;
	vblock->vlun_id = 0;
	vblock->owner_id = 0;
	vblock->flags = 0x0;

	vblock->id = vblock_id;

	prov.vblock = vblock;
	prov.lun_status = &lun_status;

	ret = ioctl(tgt, NVM_PR_GET_BLOCK_INFO, &prov);
	if (ret) {
		LNVM_DEBUG("Could not get metadata for block %lu\n", vblock_id);
		goto out;
	}

	LNVM_DEBUG("Get block medatada for block %llu. Lun: %d, bppa:%llu\n",
			vblock->id,
			vblock->vlun_id,
			vblock->bppa);
out:
	return ret;
}
#endif

int nvm_put_block(int tgt, NVM_PROV *prov)
{
#ifdef LNVM_DEBUG_ENABLED
	NVM_VBLOCK *vblock = prov->vblock;
#endif
	struct nvm_ioctl_lun_status lun_status = {
		.nr_free_blocks = 0,
		.nr_inuse_blocks = 0,
		.nr_bad_blocks = 0,
	};
	int ret = 0;

	prov->lun_status = &lun_status;

	ret = ioctl(tgt, NVM_PR_PUT_BLOCK, prov);
	if (ret) {
		LNVM_DEBUG("Could not put block %llu (bppa:%llu) to lun %d\n",
				vblock->id, vblock->bppa, vblock->vlun_id);
		goto out;
	}

	LNVM_DEBUG("Put block %llu (bbpa:%llu) to lun %d\n",
			vblock->id, vblock->bppa, vblock->vlun_id);
out:
	return ret;
}

