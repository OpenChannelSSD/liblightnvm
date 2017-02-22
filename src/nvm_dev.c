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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_debug.h>
#include <nvm_utils.h>

static struct nvm_be *nvm_backends[] = {
	&nvm_be_ioctl,
	&nvm_be_sysfs,
	NULL
};

void nvm_dev_pr(struct nvm_dev *dev)
{
	if (!dev) {
		printf("dev { NULL }\n");
		return;
	}

	printf("dev {\n");
	printf(" verid(0x%02x), beid(0x%02x),\n",
	       dev->verid, dev->beid);
	printf(" path(%s), name(%s), fd(%d),\n",
	       dev->path, dev->name, dev->fd);
	printf(" ssw(%lu), pmode(%d),\n", dev->ssw, dev->pmode);

	printf(" erase_naddrs_max(%d),", dev->erase_naddrs_max);
	printf(" read_naddrs_max(%d),", dev->read_naddrs_max);
	printf(" write_naddrs_max(%d),\n",dev->write_naddrs_max);
	       
	printf(" meta_mode(%d),\n", dev->meta_mode);
	printf(" bbts_cached(%d)\n},\n", dev->bbts_cached);
	printf("dev-"); nvm_geo_pr(&dev->geo);
	printf("dev-"); spec_ppaf_nand_pr(&dev->ppaf);
	printf("dev-"); spec_ppaf_nand_mask_pr(&dev->mask);
}

const struct nvm_geo * nvm_dev_get_geo(struct nvm_dev *dev)
{
	return &dev->geo;
}

int nvm_dev_get_verid(struct nvm_dev *dev)
{
	return dev->verid;
}

int nvm_dev_get_pmode(struct nvm_dev *dev)
{
	return dev->pmode;
}

int nvm_dev_get_meta_mode(struct nvm_dev *dev)
{
	return dev->meta_mode;
}

int nvm_dev_set_meta_mode(struct nvm_dev *dev, int meta_mode)
{
	switch (meta_mode) {
		case NVM_META_MODE_NONE:
		case NVM_META_MODE_ALPHA:
		case NVM_META_MODE_CONST:
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	dev->meta_mode = meta_mode;

	return 0;
}

int nvm_dev_get_nsid(struct nvm_dev *dev)
{
	return dev->nsid;
}

int nvm_dev_get_erase_naddrs_max(struct nvm_dev *dev)
{
	return dev->erase_naddrs_max;
}

int nvm_dev_get_read_naddrs_max(struct nvm_dev *dev)
{
	return dev->read_naddrs_max;
}

int nvm_dev_get_write_naddrs_max(struct nvm_dev *dev)
{
	return dev->write_naddrs_max;
}

int nvm_dev_set_erase_naddrs_max(struct nvm_dev *dev, int naddrs)
{
	if (naddrs > NVM_NADDR_MAX) {
		errno = EINVAL;
		return -1;
	}
	if (naddrs < 1) {
		errno = EINVAL;
		return -1;
	}
	if (naddrs % dev->geo.nplanes) {
		errno = EINVAL;
		return -1;
	}

	dev->erase_naddrs_max = naddrs;

	return 0;
}

int nvm_dev_get_bbts_cached(struct nvm_dev *dev)
{
	return dev->bbts_cached;
}

int nvm_dev_set_bbts_cached(struct nvm_dev *dev, int bbts_cached)
{
	switch(bbts_cached) {
	case 0:
	case 1:
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	dev->bbts_cached = bbts_cached;

	return 0;
}

int nvm_dev_set_read_naddrs_max(struct nvm_dev *dev, int naddrs)
{
	if (naddrs > NVM_NADDR_MAX) {
		errno = EINVAL;
		return -1;
	}
	if (naddrs < 1) {
		errno = EINVAL;
		return -1;
	}
	if (naddrs % (dev->geo.nplanes * dev->geo.nsectors)) {
		errno = EINVAL;
		return -1;
	}

	dev->read_naddrs_max = naddrs;

	return 0;
}

int nvm_dev_set_write_naddrs_max(struct nvm_dev *dev, int naddrs)
{
	if (naddrs > NVM_NADDR_MAX) {
		errno = EINVAL;
		return -1;
	}
	if (naddrs < 1) {
		errno = EINVAL;
		return -1;
	}
	if (naddrs % (dev->geo.nplanes * dev->geo.nsectors)) {
		errno = EINVAL;
		return -1;
	}

	dev->write_naddrs_max = naddrs;

	return 0;
}

struct nvm_dev * nvm_dev_openf(const char *dev_path, int NVM_UNUSED(flags)) {
	struct nvm_dev *dev = NULL;

	for (int i = 0; nvm_backends[i]; ++i) {
		dev = nvm_backends[i]->open(dev_path);
		if (dev) {
			dev->be = nvm_backends[i];
			dev->beid = i;
			break;
		}
	}

	if (!dev)
		return NULL;

	dev->bbts_cached = 0;
	dev->nbbts = dev->geo.nchannels * dev->geo.nluns;
	dev->bbts = malloc(sizeof(*dev->bbts) * dev->nbbts);
	if (!dev->bbts) {
		NVM_DEBUG("FAILED: malloc dev->bbts");
		errno = ENOMEM;
		return NULL;
	}

	for (size_t i = 0; i < dev->nbbts; ++i)
		dev->bbts[i] = NULL;

	// HACK: use naming conventions to determine nsid, fallback to hardcode
	dev->nsid = atoi(&dev_path[strlen(dev_path)-1]);
	if ((dev->nsid < 1) || (dev->nsid > 1000))
		dev->nsid = 1;

	return dev;
}

struct nvm_dev *nvm_dev_open(const char *dev_path)
{
	return nvm_dev_openf(dev_path, 0x0);
}

void nvm_dev_close(struct nvm_dev *dev)
{
	if (!dev)
		return;

	nvm_bbt_flush_all(dev, NULL);

	dev->be->close(dev);

	nvm_bbt_flush_all(dev, NULL);
	free(dev->bbts);
	free(dev);
}

// Some Misc command open char device /dev/nvme0n1
// it's don't need get and fillup geo, ppaf. etc
struct nvm_dev *char_dev_open(const char *dev_path)
{
	struct nvm_dev *dev;
	
	if (strlen(dev_path) > NVM_DEV_PATH_LEN) {
		NVM_DEBUG("FAILED: Device path too long\n");
		return NULL;
	}

	dev = malloc(sizeof(*dev));
	if (dev) {
		memset(dev, 0, sizeof(*dev));
	} else {
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

	return dev;
}

void char_dev_close(struct nvm_dev *dev)
{
	if (!dev)
		return;

	close(dev->fd);
	free(dev);
}

