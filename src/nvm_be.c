/*
 * be - Provides fall-back methods for backends which are not supported
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

static inline void _construct_ppaf_mask(struct nvm_spec_ppaf_nand *ppaf,
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
	NVM_DEBUG("nvm_be_nosys_open");
	errno = ENOSYS;
	return NULL;
}

void nvm_be_nosys_close(struct nvm_dev *NVM_UNUSED(dev))
{
	NVM_DEBUG("nvm_be_nosys_close");
	errno = ENOSYS;
	return;
}

int nvm_be_nosys_user(struct nvm_dev *NVM_UNUSED(dev),
		      struct nvm_cmd *NVM_UNUSED(cmd),
		      struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_user");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_admin(struct nvm_dev *NVM_UNUSED(dev),
		       struct nvm_cmd *NVM_UNUSED(cmd),
		       struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_admin");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_vuser(struct nvm_dev *NVM_UNUSED(dev),
		       struct nvm_cmd *NVM_UNUSED(cmd),
		       struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_vuser");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_vadmin(struct nvm_dev *NVM_UNUSED(dev),
			struct nvm_cmd *NVM_UNUSED(cmd),
			struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_vadmin");
	errno = ENOSYS;
	return -1;
}

int nvm_be_populate(struct nvm_dev *dev, int (*vadmin)(struct nvm_dev *, struct nvm_cmd *, struct nvm_ret *))
{
	struct nvm_cmd cmd = {.cdw={0}};
	struct nvm_spec_identify *idf;
	int err;

	idf = nvm_buf_alloca(4096, sizeof(*idf));
	if (!idf) {
		errno = ENOMEM;
		return -1;
	}
	memset(idf, 0, sizeof(*idf));

	cmd.vadmin.opcode = NVM_S12_OPC_IDF;	// Setup command
	cmd.vadmin.addr = (uint64_t)idf;
	cmd.vadmin.data_len = sizeof(*idf);

	err = vadmin(dev, &cmd, NULL);
	if (err) {
		nvm_buf_free(idf);
		return -1;			// NOTE: Propagate errno
	}

	switch (idf->s.verid) {
	case NVM_SPEC_VERID_12:
		dev->geo.page_nbytes = idf->s12.grp[0].fpg_sz;
		dev->geo.sector_nbytes = idf->s12.grp[0].csecs;
		dev->geo.meta_nbytes = idf->s12.grp[0].sos;

		dev->geo.nchannels = idf->s12.grp[0].num_ch;
		dev->geo.nluns = idf->s12.grp[0].num_lun;
		dev->geo.nplanes = idf->s12.grp[0].num_pln;
		dev->geo.nblocks = idf->s12.grp[0].num_blk;
		dev->geo.npages = idf->s12.grp[0].num_pg;

		dev->ppaf = idf->s12.ppaf;
		dev->mccap = idf->s12.grp[0].mccap;
		break;

	case NVM_SPEC_VERID_20:
		dev->geo.sector_nbytes = idf->s20.csecs;
		dev->geo.meta_nbytes = idf->s20.sos;
		dev->geo.page_nbytes = idf->s20.mw_min * dev->geo.sector_nbytes;

		dev->geo.nchannels = idf->s20.num_ch;
		dev->geo.nluns = idf->s20.num_lun;
		dev->geo.nplanes = idf->s20.mw_opt / idf->s20.mw_min;
		dev->geo.nblocks = idf->s20.num_chk;
		dev->geo.npages = ((idf->s20.clba * idf->s20.csecs) / dev->geo.page_nbytes) / dev->geo.nplanes;
		dev->geo.nsectors = dev->geo.page_nbytes / dev->geo.sector_nbytes;

		dev->ppaf = idf->s20.ppaf;
		dev->mccap = idf->s20.mccap;
		break;

	default:
		NVM_DEBUG("Unsupported Version ID(%d)", idf->s.verid);
		errno = ENOSYS;
		nvm_buf_free(idf);
		return -1;
	}

	dev->verid = idf->s.verid;
	_construct_ppaf_mask(&dev->ppaf, &dev->mask);

	// WARN: HOTFIX for reports of unrealisticly large OOB area
	if (dev->geo.meta_nbytes > (dev->geo.sector_nbytes * 0.1)) {
		dev->geo.meta_nbytes = 16;	// Naively hope this is right
	}
	dev->geo.nsectors = dev->geo.page_nbytes / dev->geo.sector_nbytes;

	/* Derive total number of bytes on device */
	dev->geo.tbytes = dev->geo.nchannels * dev->geo.nluns * \
			  dev->geo.nplanes * dev->geo.nblocks * \
			  dev->geo.npages * dev->geo.nsectors * \
			  dev->geo.sector_nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	dev->ssw = _ilog2(dev->geo.sector_nbytes);

	/* Derive a default plane mode */
	switch(dev->geo.nplanes) {
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
		errno = EINVAL;
		nvm_buf_free(idf);
		return -1;
	}

	dev->erase_naddrs_max = NVM_NADDR_MAX;
	dev->write_naddrs_max = NVM_NADDR_MAX;
	dev->read_naddrs_max = NVM_NADDR_MAX;

	dev->meta_mode = NVM_META_MODE_NONE;

	nvm_buf_free(idf);

	return 0;
}
