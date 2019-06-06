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

static inline void nvm_dev_vblk_opts_pr(const struct nvm_dev *dev)
{
	if (!dev) {
		printf("vblk_opts: ~\n");
		return;
	}

	printf("vblk_opts:\n");
	printf("  pmode: '%s'\n", nvm_pmode_str(nvm_dev_get_pmode(dev)));
	printf("  erase_naddrs_max: %d\n", nvm_dev_get_erase_naddrs_max(dev));
	printf("  read_naddrs_max: %d\n", nvm_dev_get_read_naddrs_max(dev));
	printf("  write_naddrs_max: %d\n", nvm_dev_get_write_naddrs_max(dev));
	printf("  meta_mode: %d\n", nvm_dev_get_meta_mode(dev));
}

static inline void nvm_dev_cmd_opts_pr(const struct nvm_dev *dev)
{
	if (!dev) {
		printf("cmd_opts: ~\n");
		return;
	}

	printf("cmd_opts:\n");
	printf("  mask: '"NVM_I32_FMT"'\n", NVM_I32_TO_STR(dev->cmd_opts));
	printf("  iomd: '%s'\n",
	       (dev->cmd_opts & NVM_CMD_SYNC) ? "SYNC" : "ASYNC");
	printf("  addr: '%s'\n",
	       (dev->cmd_opts & NVM_CMD_SCALAR) ? "SCALAR" : "VECTOR");
	printf("  plod: '%s'\n",
	       (dev->cmd_opts & NVM_CMD_PRP) ? "PRP" : "SGL");
}

void nvm_dev_attr_pr(const struct nvm_dev *dev)
{
	if (!dev) {
		printf("attr: ~\n");
		return;
	}

	printf("attr:\n");
	printf("  verid: 0x%02x\n", nvm_dev_get_verid(dev));
	printf("  be_id: 0x%02x\n", nvm_dev_get_be_id(dev));
	printf("  be_name: '%s'\n", dev->be->name);
	printf("  name: '%s'\n", nvm_dev_get_name(dev));
	printf("  path: '%s'\n", nvm_dev_get_path(dev));
	printf("  fd: %d\n", nvm_dev_get_fd(dev));
	printf("  ssw: %"PRIu64"\n", dev->ssw);
	printf("  mccap: '"NVM_I32_FMT"'\n",
	       NVM_I32_TO_STR(nvm_dev_get_mccap(dev)));
	printf("  bbts_cached: %d\n", nvm_dev_get_bbts_cached(dev));
	printf("  quirks: '"NVM_I8_FMT"'\n",
	       NVM_I8_TO_STR(nvm_dev_get_quirks(dev)));
}

void nvm_dev_pr(const struct nvm_dev *dev)
{
	if (!dev) {
		printf("dev: ~\n");
		return;
	}

	printf("dev_"); nvm_dev_attr_pr(dev);
	printf("dev_"); nvm_geo_pr(&dev->geo);
	printf("dev_"); nvm_dev_cmd_opts_pr(dev);
	printf("dev_"); nvm_dev_vblk_opts_pr(dev);

	switch(nvm_dev_get_verid(dev)) {
	case NVM_SPEC_VERID_12:
		printf("dev_"); nvm_spec_ppaf_nand_pr(&dev->ppaf);
		printf("dev_"); nvm_spec_ppaf_nand_mask_pr(&dev->mask);
		printf("dev_"); nvm_spec_lbaf_pr(NULL);
		printf("dev_"); nvm_spec_lbaz_pr(NULL);
		printf("dev_"); nvm_spec_lbam_pr(NULL);
		break;

	case NVM_SPEC_VERID_20:
		printf("dev_"); nvm_spec_ppaf_nand_pr(NULL);
		printf("dev_"); nvm_spec_ppaf_nand_mask_pr(NULL);
		printf("dev_"); nvm_spec_lbaf_pr(&dev->lbaf);
		printf("dev_"); nvm_spec_lbaz_pr(&dev->lbaz);
		printf("dev_"); nvm_spec_lbam_pr(&dev->lbam);
		break;
	}
}

const struct nvm_geo * nvm_dev_get_geo(const struct nvm_dev *dev)
{
	return &dev->geo;
}

const struct nvm_nvme_ns *nvm_dev_get_ns(const struct nvm_dev *dev)
{
	return &dev->ns;
}

int nvm_dev_get_ws_min(const struct nvm_dev *dev)
{
	switch(dev->verid) {
	case NVM_SPEC_VERID_12:
		return dev->geo.g.nsectors;
	case NVM_SPEC_VERID_20:
		return dev->idfy.s20.wrt.ws_min;
	}

	errno = EINVAL;
	return -1;
}

int nvm_dev_get_ws_opt(const struct nvm_dev *dev)
{
	switch(dev->verid) {
	case NVM_SPEC_VERID_12:
		return dev->geo.g.nplanes * dev->geo.g.nsectors;
	case NVM_SPEC_VERID_20:
		return dev->idfy.s20.wrt.ws_opt;
	}

	errno = EINVAL;
	return -1;
}

int nvm_dev_get_mw_cunits(const struct nvm_dev *dev)
{
	switch(dev->verid) {
	case NVM_SPEC_VERID_20:
		return dev->idfy.s20.wrt.mw_cunits;

	case NVM_SPEC_VERID_12:
	default:
		errno = EINVAL;
		return -1;
	}
}

int nvm_dev_get_verid(const struct nvm_dev *dev)
{
	return dev->verid;
}

int nvm_dev_get_fd(const struct nvm_dev *dev)
{
	return dev->fd;
}

int nvm_dev_get_be_id(const struct nvm_dev *dev)
{
	return dev->be->id;
}

const char *nvm_dev_get_name(const struct nvm_dev *dev)
{
	return dev->name;
}

const char *nvm_dev_get_path(const struct nvm_dev *dev)
{
	return dev->path;
}

uint32_t nvm_dev_get_mccap(const struct nvm_dev *dev)
{
	return dev->mccap;
}

int nvm_dev_get_quirks(const struct nvm_dev *dev)
{
	return dev->quirks;
}

int nvm_dev_set_quirks(struct nvm_dev *dev, int quirks)
{
	dev->quirks = quirks;
	return 0;
}

int nvm_dev_get_pmode(const struct nvm_dev *dev)
{
	return dev->vblk_opts.pmode;
}

int nvm_dev_set_pmode(struct nvm_dev *dev, int pmode)
{
	switch (pmode) {
	case NVM_FLAG_PMODE_QUAD:
		if (dev->geo.nplanes < 4) {
			errno = EINVAL;
			return -1;
		}
		/* FALLTHRU */
	case NVM_FLAG_PMODE_DUAL:
		if (dev->geo.nplanes < 2) {
			errno = EINVAL;
			return -1;
		}
		/* FALLTHRU */
	case NVM_FLAG_PMODE_SNGL:
		dev->vblk_opts.pmode = pmode;
		return 0;

	default:
		errno = EINVAL;
		return -1;
	}
}

int nvm_dev_get_meta_mode(const struct nvm_dev *dev)
{
	return dev->vblk_opts.meta_mode;
}

int nvm_dev_set_meta_mode(struct nvm_dev *dev, int meta_mode)
{
	switch (meta_mode) {
	case NVM_META_MODE_NONE:
		dev->vblk_opts.meta_mode = NVM_META_MODE_NONE;
		return 0;
	case NVM_META_MODE_ALPHA:
		dev->vblk_opts.meta_mode = NVM_META_MODE_ALPHA;
		return 0;
	case NVM_META_MODE_CONST:
		dev->vblk_opts.meta_mode = NVM_META_MODE_CONST;
		return 0;

	default:
		errno = EINVAL;
		return -1;
	}
}

const struct nvm_spec_ppaf_nand *nvm_dev_get_ppaf(const struct nvm_dev *dev)
{
	return &dev->ppaf;
}

const struct nvm_spec_ppaf_nand_mask *nvm_dev_get_ppaf_mask(const struct nvm_dev *dev)
{
	return &dev->mask;
}

const struct nvm_spec_lbaf *nvm_dev_get_lbaf(const struct nvm_dev *dev)
{
	return &dev->lbaf;
}

int nvm_dev_get_nsid(const struct nvm_dev *dev)
{
	return dev->nsid;
}

int nvm_dev_get_erase_naddrs_max(const struct nvm_dev *dev)
{
	return dev->vblk_opts.erase_naddrs_max;
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

	dev->vblk_opts.erase_naddrs_max = naddrs;

	return 0;
}

int nvm_dev_get_read_naddrs_max(const struct nvm_dev *dev)
{
	return dev->vblk_opts.read_naddrs_max;
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
	if (dev->vblk_opts.pmode && (naddrs % (dev->geo.nplanes))) {
		errno = EINVAL;
		return -1;
	}

	dev->vblk_opts.read_naddrs_max = naddrs;

	return 0;
}

int nvm_dev_get_write_naddrs_max(const struct nvm_dev *dev)
{
	return dev->vblk_opts.write_naddrs_max;
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

	if (dev->verid != NVM_SPEC_VERID_20) {
		if (dev->vblk_opts.pmode && (naddrs % (dev->geo.nplanes * dev->geo.nsectors))) {
			errno = EINVAL;
			return -1;
		}
		if (naddrs % (dev->geo.nsectors)) {
			errno = EINVAL;
			return -1;
		}
	}

	dev->vblk_opts.write_naddrs_max = naddrs;

	return 0;
}

int nvm_dev_get_bbts_cached(const struct nvm_dev *dev)
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

struct nvm_dev * nvm_dev_openf(const char *dev_path, int flags) {
	struct nvm_dev *dev = NULL;

	dev = nvm_be_factory(dev_path, flags);
	if (!dev) {
		errno = errno ? errno : EIO;
		NVM_DEBUG("FAILED: no backend for ident: %s with flags: %d",
			  dev_path, flags);
		return NULL;
	}

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

	dev->cmd_opts = 0;	// Setup CMD options

	if (flags & NVM_CMD_MASK_IOMD) {
		NVM_DEBUG("setting IOMD using flags");
		dev->cmd_opts |= flags & NVM_CMD_MASK_IOMD;
	} else {
		NVM_DEBUG("setting IOMD using def: 0x%x", NVM_CMD_DEF_IOMD);
		dev->cmd_opts |= NVM_CMD_DEF_IOMD;
	}

	if (flags & NVM_CMD_MASK_ADDR) {
		NVM_DEBUG("setting ADDR using flags");
		dev->cmd_opts |= flags & NVM_CMD_MASK_ADDR;
	} else {
		NVM_DEBUG("setting ADDR using def: 0x%x", NVM_CMD_DEF_ADDR);
		dev->cmd_opts |= NVM_CMD_DEF_ADDR;
	}

	if (flags & NVM_CMD_MASK_PLOD) {
		NVM_DEBUG("setting PLOD using flags");
		dev->cmd_opts |= flags & NVM_CMD_MASK_PLOD;
	} else {
		NVM_DEBUG("setting PLOD using def: 0x%x", NVM_CMD_DEF_PLOD);
		dev->cmd_opts |= NVM_CMD_DEF_PLOD;
	}

	NVM_DEBUG("cmd_opts: 0x%x", dev->cmd_opts);

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

	free(dev->bbts);
	free(dev);
}
