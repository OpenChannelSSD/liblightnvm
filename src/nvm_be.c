/*
 * be - Provides fall-back methods and helper functions for actual backends
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
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

static inline uint64_t _ilog2(uint64_t x)
{
	uint64_t val = 0;

	while (x >>= 1)
		val++;

	return val;
}

static inline void construct_ppaf_mask(struct nvm_spec_ppaf_nand *ppaf,
					struct nvm_spec_ppaf_nand_mask *mask)
{
	for (int i = 0 ; i < 12; ++i) {
		if ((i % 2)) {
			// i-1 = offset
			// i = width
			mask->a[i/2] = (((uint64_t)1<< ppaf->a[i])-1) << ppaf->a[i-1];
		}
	}
}

struct nvm_dev* nvm_be_nosys_open(const char *NVM_UNUSED(dev_path),
				  int NVM_UNUSED(flags))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

void nvm_be_nosys_close(struct nvm_dev *NVM_UNUSED(dev))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return;
}

struct nvm_spec_idfy *nvm_be_nosys_idfy(struct nvm_dev *NVM_UNUSED(dev),
					struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

		
struct nvm_spec_rprt *nvm_be_nosys_rprt(struct nvm_dev *NVM_UNUSED(dev),
					struct nvm_addr NVM_UNUSED(addr),
					uint16_t NVM_UNUSED(naddrs),
					int NVM_UNUSED(flags),
				        struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

struct nvm_spec_bbt *nvm_be_nosys_gbbt(struct nvm_dev *NVM_UNUSED(dev),
				       struct nvm_addr NVM_UNUSED(addr),
				       struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return NULL;
}

int nvm_be_nosys_sbbt(struct nvm_dev *NVM_UNUSED(dev),
		      struct nvm_addr *NVM_UNUSED(addrs),
		      int NVM_UNUSED(naddrs), uint16_t NVM_UNUSED(flags),
		      struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_erase(struct nvm_dev *NVM_UNUSED(dev),
		       struct nvm_addr *NVM_UNUSED(addrs),
		       int NVM_UNUSED(naddrs),
		       uint16_t NVM_UNUSED(flags),
		       struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_write(struct nvm_dev *NVM_UNUSED(dev),
		       struct nvm_addr *NVM_UNUSED(addrs),
		       int NVM_UNUSED(naddrs),
		       void *NVM_UNUSED(data), void *NVM_UNUSED(meta),
		       uint16_t NVM_UNUSED(flags),
		       struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_read(struct nvm_dev *NVM_UNUSED(dev),
		      struct nvm_addr *NVM_UNUSED(addrs),
		      int NVM_UNUSED(naddrs), void *NVM_UNUSED(data),
		      void *NVM_UNUSED(meta), uint16_t NVM_UNUSED(flags),
		      struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_copy(struct nvm_dev *NVM_UNUSED(dev),
		      struct nvm_addr NVM_UNUSED(src[]),
		      struct nvm_addr NVM_UNUSED(dst[]),
		      int NVM_UNUSED(naddrs),
		      uint16_t NVM_UNUSED(flags),
		      struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("FAILED: not implemented(possibly intentionally)");
	errno = ENOSYS;
	return -1;
}

int nvm_be_split_dpath(const char *dev_path, char *nvme_name, int *nsid)
{
	const char prefix[] = "/dev/nvme";
	const size_t prefix_len = strlen(prefix);
	int val;

	if (strlen(dev_path) < prefix_len + 3) {
		errno = EINVAL;
		return -1;
	}

	if (strncmp(prefix, dev_path, prefix_len)) {
		errno = EINVAL;
		return -1;
	}

	val = atoi(&dev_path[strlen(dev_path)-1]);
	if ((val < 1) || (val > 1024)) {
		errno = EINVAL;
		return -1;
	}

	strncpy(nvme_name, dev_path + 5, 5);
	*nsid = val;

	return 0;
}

int nvm_be_populate(struct nvm_dev *dev, struct nvm_be *be)
{
	struct nvm_geo *geo = &dev->geo;
	struct nvm_spec_idfy *idfy = NULL;

	idfy = be->idfy(dev, NULL);
	if (!idfy) {
		NVM_DEBUG("FAILED: nvm_cmd_idfy");
		return -1; // NOTE: Propagate errno
	}

	// Handle IDFY (draft / pre 2.0) reporting 2.0, mark as 1.3
	if (idfy->s.verid == NVM_SPEC_VERID_20 && !(
		idfy->s20.lbaf.pugrp_len &&
		idfy->s20.lbaf.punit_len &&
		idfy->s20.lbaf.chunk_len &&
		idfy->s20.lbaf.sectr_len)) {
		idfy->s.verid = NVM_SPEC_VERID_13;
	}

	#ifdef NVM_DEBUG_ENABLED
	nvm_spec_idfy_pr(idfy);
	#endif

	switch (idfy->s.verid) {
	case NVM_SPEC_VERID_12:
		geo->page_nbytes = idfy->s12.grp[0].fpg_sz;
		geo->sector_nbytes = idfy->s12.grp[0].csecs;
		geo->meta_nbytes = idfy->s12.grp[0].sos;

		geo->nchannels = idfy->s12.grp[0].num_ch;
		geo->nluns = idfy->s12.grp[0].num_lun;
		geo->nplanes = idfy->s12.grp[0].num_pln;
		geo->nblocks = idfy->s12.grp[0].num_blk;
		geo->npages = idfy->s12.grp[0].num_pg;

		dev->mccap = idfy->s12.grp[0].mccap;

		dev->ppaf = idfy->s12.ppaf;
		construct_ppaf_mask(&dev->ppaf, &dev->mask);

		break;

	case NVM_SPEC_VERID_13:
		geo->sector_nbytes = idfy->s13.lgeo.nbytes;
		geo->meta_nbytes = idfy->s13.lgeo.nbytes_oob;
		geo->page_nbytes = idfy->s13.wrt.ws_min * geo->sector_nbytes;

		geo->nchannels = idfy->s13.lgeo.npugrp;
		geo->nluns = idfy->s13.lgeo.npunit;
		geo->nplanes = idfy->s13.wrt.ws_opt / idfy->s13.wrt.ws_min;
		geo->nblocks = idfy->s13.lgeo.nchunk;
		geo->npages = ((idfy->s13.lgeo.nsectr * idfy->s13.lgeo.nbytes) / geo->page_nbytes) / geo->nplanes;
		geo->nsectors = geo->page_nbytes / geo->sector_nbytes;

		dev->mccap = idfy->s13.mccap;

		dev->ppaf = idfy->s13.ppaf;
		construct_ppaf_mask(&dev->ppaf, &dev->mask);

		dev->lbaf = idfy->s20.lbaf;
		dev->lgeo = idfy->s20.lgeo;

		break;

	case NVM_SPEC_VERID_20:
		geo->sector_nbytes = idfy->s20.lgeo.nbytes;
		geo->meta_nbytes = idfy->s20.lgeo.nbytes_oob;
		geo->page_nbytes = idfy->s20.wrt.ws_min * geo->sector_nbytes;

		geo->nchannels = idfy->s20.lgeo.npugrp;
		geo->nluns = idfy->s20.lgeo.npunit;
		geo->nplanes = idfy->s20.wrt.ws_opt / idfy->s20.wrt.ws_min;
		geo->nblocks = idfy->s20.lgeo.nchunk;
		geo->npages = ((idfy->s20.lgeo.nsectr * idfy->s20.lgeo.nbytes) / geo->page_nbytes) / geo->nplanes;
		geo->nsectors = geo->page_nbytes / geo->sector_nbytes;

		dev->mccap = idfy->s20.mccap;

		// NOTE: NO PPAF FOR 2.0 spec.
		dev->lbaf = idfy->s20.lbaf;
		dev->lgeo = idfy->s20.lgeo;

		break;

	default:
		NVM_DEBUG("Unsupported Version ID(%d)", idfy->s.verid);
		errno = ENOSYS;
		nvm_buf_free(idfy);
		return -1;
	}

	dev->verid = idfy->s.verid;

	nvm_buf_free(idfy);

	return 0;
}

int nvm_be_populate_quirks(struct nvm_dev *dev, const char serial[])
{
	const int serial_len = strlen(serial);

	if (!strncmp("CX8800ES", serial, 8 < serial_len ? 8 : serial_len)) {
		dev->quirks = NVM_QUIRK_PMODE_ERASE_RUNROLL;

		switch(dev->verid) {
		case NVM_SPEC_VERID_12:
			dev->quirks |= NVM_QUIRK_OOB_2LRG;
			break;

		case NVM_SPEC_VERID_13:
		case NVM_SPEC_VERID_20:
			dev->quirks |= NVM_QUIRK_OOB_READ_1ST4BYTES_NULL;
			break;
		}

	} else {
		NVM_DEBUG("INFO: no known quirks for serial: %s", serial);
		return 0;
	}

	// HOTFIX: for reports of unrealisticly large OOB area
	if ((dev->quirks & NVM_QUIRK_OOB_2LRG) &&
	    (dev->geo.meta_nbytes > (dev->geo.sector_nbytes * 0.1))) {
		dev->geo.meta_nbytes = 16; // Naively hope this is right
	}
	
	return 0;
}

int nvm_be_populate_derived(struct nvm_dev *dev)
{
	struct nvm_geo *geo = &dev->geo;

	geo->nsectors = geo->page_nbytes / geo->sector_nbytes;

	/* Derive total number of bytes on device */
	geo->tbytes = geo->nchannels * geo->nluns * \
			geo->nplanes * geo->nblocks * \
			geo->npages * geo->nsectors * \
			geo->sector_nbytes;

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
		NVM_DEBUG("FAILED: invalid geo->>nplanes: %lu",
			  geo->nplanes);
		errno = EINVAL;
		return -1;
	}

	dev->erase_naddrs_max = NVM_NADDR_MAX;
	dev->write_naddrs_max = NVM_NADDR_MAX;
	dev->read_naddrs_max = NVM_NADDR_MAX;

	dev->meta_mode = NVM_META_MODE_NONE;

	return 0;
}
