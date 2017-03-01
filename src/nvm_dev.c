/*
 * dev - Device functions
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
#include <stdio.h>
#include <errno.h>
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

const char *nvm_pmode_str(int pmode) {
	switch (pmode) {
	case NVM_FLAG_PMODE_SNGL:
		return "SNGL";
	case NVM_FLAG_PMODE_DUAL:
		return "DUAL";
	case NVM_FLAG_PMODE_QUAD:
		return "QUAD";
	default:
		return "UNKN";
	}
}

void nvm_dev_pr(struct nvm_dev *dev)
{
	if (!dev) {
		printf("dev { NULL }\n");
		return;
	}

	printf("dev {\n");
	printf(" verid(0x%02x), be->id(0x%02x),\n",
	       dev->verid, dev->be->id);
	printf(" path(%s), name(%s), fd(%d),\n",
	       dev->path, dev->name, dev->fd);
	printf(" ssw(%lu), pmode(%s),\n", dev->ssw, nvm_pmode_str(dev->pmode));

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

int nvm_dev_get_be_id(struct nvm_dev *dev)
{
	return dev->be->id;
}

int nvm_dev_get_pmode(struct nvm_dev *dev)
{
	return dev->pmode;
}

int nvm_dev_set_pmode(struct nvm_dev *dev, int pmode)
{
	switch (pmode) {
	case NVM_FLAG_PMODE_QUAD:
		if (dev->geo.nplanes < 4) {
			errno = EINVAL;
			return -1;
		}
	case NVM_FLAG_PMODE_DUAL:
		if (dev->geo.nplanes < 2) {
			errno = EINVAL;
			return -1;
		}
	case NVM_FLAG_PMODE_SNGL:
		dev->pmode = pmode;
		return 0;

	default:
		errno = EINVAL;
		return -1;
	}
}

int nvm_dev_get_meta_mode(struct nvm_dev *dev)
{
	return dev->meta_mode;
}

int nvm_dev_set_meta_mode(struct nvm_dev *dev, int meta_mode)
{
	switch (meta_mode) {
	case NVM_META_MODE_NONE:
		dev->meta_mode = NVM_META_MODE_NONE;
		return 0;
	case NVM_META_MODE_ALPHA:
		dev->meta_mode = NVM_META_MODE_ALPHA;
		return 0;
	case NVM_META_MODE_CONST:
		dev->meta_mode = NVM_META_MODE_CONST;
		return 0;

	default:
		errno = EINVAL;
		return -1;
	}
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

struct nvm_dev * nvm_dev_openf(const char *dev_path, int flags) {
	struct nvm_dev *dev = NULL;

	int be = flags & NVM_BE_ALL;

	for (int i = 0; nvm_backends[i]; ++i) {
		if (be && !(nvm_backends[i]->id & be))
			continue;

		dev = nvm_backends[i]->open(dev_path);
		if (dev) {
			dev->be = nvm_backends[i];
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

