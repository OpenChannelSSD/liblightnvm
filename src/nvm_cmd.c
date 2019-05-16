/*
 * cmd - Encapsulation of command construction, submission, and completion
 *
 * Copyright (C) Simon A. F. Lund <slund@cnexlabs.com>
 * Copyright (C) Klaus B. A. Jensen <klaus.jensen@cnexlabs.com>
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
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_cmd.h>
#include <nvm_sgl.h>

int nvm_cmd_is_scalar(uint16_t opcode)
{
	switch (opcode) {
	case NVM_DOPC_SCALAR_WRITE:
	case NVM_DOPC_SCALAR_READ:
	case NVM_DOPC_SCALAR_ERASE:
		return 1;
	}

	return 0;
}

void nvm_cmd_wrap_term(struct nvm_cmd_wrap *wrap)
{
	nvm_buf_free(wrap->dev, wrap->dsmr_dma);
	nvm_buf_free(wrap->dev, wrap->addrs_dma);
	nvm_buf_free(wrap->dev, wrap->dst_dma);
	free(wrap);
}

void nvm_cmd_wrap_cpl(struct nvm_cmd_wrap *wrap,
		      const struct nvm_nvme_cpl *cpl)
{
	if (wrap->ret) {
		wrap->ret->result.cdw0 = cpl->cdw0;
		wrap->ret->status = cpl->status.sc
					| (cpl->status.sct << 8)
					| (cpl->status.m   << 13)
					| (cpl->status.dnr << 14);
	}

	wrap->completed = (cpl->status.sc != 0 || cpl->status.sct != 0) ? -1 : 1;
}

static inline void cmd_wrap_setup_sgl(struct nvm_cmd_wrap *wrap, void *data,
				      void *meta, int flags)
{
	struct nvm_sgl *sgl = data;
	uint64_t phys;

	if (!(flags & NVM_CMD_SGL)) {
		return;
	}

	wrap->cmd.psdt = NVM_NVME_PSDT_SGL_MPTR_CONTIGUOUS;

	if (sgl->ndescr == 1) {
		wrap->cmd.dptr.sgl = sgl->descriptors[0];
	} else {
		nvm_buf_vtophys(wrap->dev, sgl->descriptors, &phys);

		wrap->cmd.dptr.sgl.unkeyed.type = NVM_NVME_SGL_DESCR_TYPE_LAST_SEGMENT;
		wrap->cmd.dptr.sgl.unkeyed.len = sgl->ndescr * sizeof(struct nvm_nvme_sgl_descriptor);
		wrap->cmd.dptr.sgl.addr = phys;
	}

	wrap->data = NULL;
	wrap->data_len = 0;

	if (flags & NVM_CMD_SGL_META) {
		wrap->cmd.psdt = NVM_NVME_PSDT_SGL_MPTR_SGL;

		sgl = meta;

		nvm_buf_vtophys(wrap->dev, sgl->descriptors, &phys);
		if (sgl->ndescr == 1) {
			wrap->cmd.mptr = phys;
		} else {
			sgl->indirect->unkeyed.type = NVM_NVME_SGL_DESCR_TYPE_LAST_SEGMENT;
			sgl->indirect->unkeyed.len = sgl->ndescr * sizeof(struct nvm_nvme_sgl_descriptor);
			sgl->indirect->addr = phys;

			nvm_buf_vtophys(wrap->dev, sgl->indirect, &phys);
			wrap->cmd.mptr = phys;
		}

		wrap->meta = NULL;
		wrap->meta_len = 0;
	}
}

/**
 * Setup submission entry and virt_allocate DMA memory for the given opcode
 */
struct nvm_cmd_wrap *nvm_cmd_wrap_setup(struct nvm_dev *dev, int opcode,
					void *data, void *meta,
					struct nvm_addr addrs[],
					struct nvm_addr dst[],
					int naddrs,
					int flags,
					struct nvm_ret *ret)
{
	const struct nvm_geo *geo = &dev->geo;
	struct nvm_cmd_wrap *wrap;

	wrap = calloc(1, sizeof(*wrap));
	if (!wrap) {
		NVM_DEBUG("FAILED: allocating wrap");
		// Propagate errno from calloc
		return NULL;
	}

	wrap->dev = dev;
	wrap->ret = ret;
	wrap->completed = 0;

	wrap->data = data;
	wrap->meta = meta;
	wrap->naddrs = naddrs;
	wrap->addrs_len = naddrs * sizeof(uint64_t);

	wrap->cmd.opcode = opcode;
	wrap->cmd.nsid = nvm_dev_get_nsid(dev);

	if (NVM_DOPC_SCALAR_ERASE == opcode) {
		wrap->dsmr_len = sizeof(*wrap->dsmr_dma) * naddrs;
		wrap->dsmr_dma = nvm_buf_alloc(dev, wrap->dsmr_len, NULL);
		if (!wrap->dsmr_dma) {
			NVM_DEBUG("FAILED: nvm_buf_alloc of DSM range");
			goto failed;
		}

		wrap->data = wrap->dsmr_dma;
		wrap->data_len = wrap->dsmr_len;

		for(int idx = 0; idx < naddrs; ++idx) {
			const uint64_t slba = nvm_addr_gen2dev(dev, addrs[idx]);

			wrap->dsmr_dma[idx].cattr = 0;
			wrap->dsmr_dma[idx].nlb = geo->l.nsectr;
			wrap->dsmr_dma[idx].slba = slba;
		}

		wrap->cmd.dsm.nr = naddrs - 1;
		wrap->cmd.dsm.ad = 1;

		return wrap;
	}

	switch (dev->verid) {
	case NVM_SPEC_VERID_12:
		wrap->cmd.s12.naddrs = naddrs - 1;
		wrap->cmd.s12.control = flags;

		wrap->data_len = data ? geo->g.sector_nbytes * naddrs : 0;
		wrap->meta_len = geo->g.meta_nbytes * naddrs;
		break;

	case NVM_SPEC_VERID_20:
		if (nvm_cmd_is_scalar(opcode)) {
			wrap->cmd.nvme.naddrs = naddrs - 1;
		} else {
			wrap->cmd.s20.naddrs = naddrs - 1;
		}

		wrap->data_len = data ? geo->l.nbytes * naddrs : 0;
		wrap->meta_len = meta ? geo->l.nbytes_oob * naddrs : 0;
		break;

	default:
		NVM_DEBUG("FAILED: dev->verid: %d", dev->verid);
		errno = EINVAL;
		goto failed;
	}

	// Special-case handling of VECTOR_ERASE
	//
	// - overwrite meta_len
	// - Assign mptr since it will most likely not be assigned by backend
	if ((opcode == NVM_DOPC_VECTOR_ERASE) && meta) {
		wrap->meta_len = naddrs * sizeof(struct nvm_spec_rprt_descr);
		if (nvm_buf_vtophys(dev, meta, &wrap->cmd.mptr)) {
			NVM_DEBUG("FAILED: nvm_buf_vtophys");
			goto failed;
		}
	}

	cmd_wrap_setup_sgl(wrap, data, meta, flags);

	switch (opcode) {
	case NVM_DOPC_SCALAR_WRITE:
	case NVM_DOPC_SCALAR_READ:
		wrap->cmd.addrs = nvm_addr_gen2dev(dev, addrs[0]);
		/* FALLTHRU */

	case NVM_DOPC_SCALAR_ERASE:
		return wrap;		// For SCALAR we are done preparing

	default:			// For VECTOR we continue
		break;
	}

	// DMA allocation and address translation for VECTOR commands
	if (naddrs > 1) {
		uint64_t addrs_phys = 0;

		wrap->addrs_dma = nvm_buf_alloc(dev, wrap->addrs_len,
						&addrs_phys);
		if (!wrap->addrs_dma) {
			NVM_DEBUG("FAILED: nvm_buf_alloc(addrs)");
			goto failed;
		}

		for (int i = 0; i < naddrs; ++i) {
			wrap->addrs_dma[i] = nvm_addr_gen2dev(dev, addrs[i]);
		}

		wrap->cmd.addrs = addrs_phys;
	} else {
		wrap->cmd.addrs = nvm_addr_gen2dev(dev, addrs[0]);
	}

	if (dst) {		// Addrs. for COPY(DST)
		if (naddrs > 1) {
			uint64_t dst_phys = 0;

			wrap->dst_dma = nvm_buf_alloc(dev, wrap->addrs_len,
							&dst_phys);
			if (!wrap->dst_dma) {
				NVM_DEBUG("FAILED: nvm_buf_alloc(dst)");
				goto failed;
			}

			for (int i = 0; i < naddrs; ++i) {
				wrap->dst_dma[i] = nvm_addr_gen2dev(dev, dst[i]);
			}
			wrap->cmd.addrs_dst = dst_phys;
		} else {
			wrap->cmd.addrs_dst = nvm_addr_gen2dev(dev, dst[0]);
		}
	}

	return wrap;

failed:
	nvm_cmd_wrap_term(wrap);
	// Propagate errno
	return NULL;
}

struct nvm_cmd_wrap *nvm_cmd_wrap_pass(struct nvm_dev *dev,
				       struct nvm_nvme_cmd *NVM_UNUSED(cmd),
				       void *data, size_t data_nbytes,
				       void *meta, size_t meta_nbytes,
				       int NVM_UNUSED(flags),
				       struct nvm_ret *ret)
{
	struct nvm_cmd_wrap *wrap = NULL;

	wrap = calloc(1, sizeof(*wrap));
	if (!wrap) {
		NVM_DEBUG("FAILED: allocating wrap");
		// Propagate errno from calloc
		return NULL;
	}

	wrap->dev = dev;
	wrap->data = data;
	wrap->data_len = data_nbytes;
	wrap->meta = meta;
	wrap->meta_len = meta_nbytes;
	wrap->ret = ret;
	wrap->completed = 0;

	return wrap;
}

int nvm_cmd_pass(struct nvm_dev *dev, struct nvm_nvme_cmd *cmd,
		 void *data, size_t data_nbytes,
		 void *meta, size_t meta_nbytes,
		 int flags, struct nvm_ret *ret)
{
	return dev->be->pass(dev, cmd, data, data_nbytes, meta, meta_nbytes,
			     flags, ret);
}

struct nvm_spec_idfy *nvm_cmd_idfy(struct nvm_dev *dev, struct nvm_ret *ret)
{
	return dev->be->idfy(dev, ret);
}

struct nvm_spec_rprt *nvm_cmd_rprt(struct nvm_dev *dev, struct nvm_addr *addr,
				   int opt, struct nvm_ret *ret)
{
	return dev->be->rprt(dev, addr, opt, ret);
}

int nvm_cmd_rprt_arbs(struct nvm_dev *dev, int cs, int naddrs,
		      struct nvm_addr *addrs)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	int tpunit = geo->l.npugrp * geo->l.npunit;
	const int arb = rand();

	if ((naddrs < 0) || (naddrs > tpunit)) {
		errno = EINVAL;
		return -1;
	}

	for (int idx = 0; idx < naddrs; ++idx) {
		struct nvm_spec_rprt *rprt = NULL;
		struct nvm_addr addr = { .val = 0 };
		size_t cur = (idx + arb) % naddrs;
		size_t des_idx;

		addr.l.pugrp = cur % geo->l.npugrp;
		addr.l.punit = (cur / geo->l.npugrp) % geo->l.npunit;

		rprt = nvm_cmd_rprt(dev, &addr, 0x0, NULL);	// Grab RPRT
		if (!(rprt && (rprt->ndescr == geo->l.nchunk))) {
			nvm_buf_free(dev, rprt);
			errno = EINVAL;
			return -1;
		}

		for (des_idx = 0; des_idx < rprt->ndescr; ++des_idx) {
			int des_cur = (des_idx + arb) % rprt->ndescr;

			if (rprt->descr[des_cur].cs != cs)
				continue;

			addrs[idx].val = addr.val;
			addrs[idx].l.chunk = des_cur;
			break;
		}

		if (des_idx == rprt->ndescr) {			// No chunk !
			nvm_buf_free(dev, rprt);
			errno = EINVAL;
			return -1;
		}

		nvm_buf_free(dev, rprt);
	}

	return 0;
}

struct nvm_spec_bbt *nvm_cmd_gbbt(struct nvm_dev *dev, struct nvm_addr addr,
				  struct nvm_ret *ret)
{
	return dev->be->gbbt(dev, addr, ret);
}

int nvm_cmd_gbbt_arbs(struct nvm_dev *dev, int bs, int naddrs,
		      struct nvm_addr *addrs)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	int tluns = geo->g.nchannels * geo->g.nluns;
	int arb = rand();

	if ((naddrs < 0) || naddrs > tluns) {
		errno = EINVAL;
		return -1;
	}

	for (size_t idx = 0; idx < (size_t)naddrs; ++idx) {
		struct nvm_spec_bbt *bbt = NULL;
		struct nvm_addr addr = { .val = 0 };
		size_t cur = (idx + arb) % naddrs;
		size_t blk_idx;

		addr.g.ch = cur % geo->g.nchannels;
		addr.g.lun = (cur / geo->g.nchannels) % geo->g.nluns;

		bbt = nvm_cmd_gbbt(dev, addr, NULL);		// Grab BBT
		if (!bbt)
			return -1;

		for (blk_idx = 0; idx < geo->g.nblocks; ++idx) {
			size_t blk_cur = (blk_idx + arb) % geo->g.nblocks;
			int state = 0;

			// Accumulate plane state
			for (uint32_t idx = blk_cur * geo->g.nplanes;
			     idx < blk_cur * geo->g.nplanes + geo->g.nplanes;
				++idx)
				state |= bbt->blk[idx];

			// Check state
			if (bs != state)
				continue;

			addrs[idx].val = addr.val;
			addrs[idx].g.blk = blk_cur;
			break;
		}
		if (blk_idx == geo->g.nblocks)
			return -1;				// No block!
	}

	return 0;
}

int nvm_cmd_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs, int naddrs,
		 uint16_t flags, struct nvm_ret *ret)
{
	return dev->be->sbbt(dev, addrs, naddrs, flags, ret);
}

int nvm_cmd_gfeat(struct nvm_dev *dev, enum nvm_nvme_feat_id id, union nvm_nvme_feat *feat,
		  struct nvm_ret *ret)
{
	return dev->be->gfeat(dev, id, feat, ret);
}

int nvm_cmd_sfeat(struct nvm_dev *dev, enum nvm_nvme_feat_id id,
		  const union nvm_nvme_feat *feat, struct nvm_ret *ret)
{
	return dev->be->sfeat(dev, id, feat, ret);
}

int nvm_cmd_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *meta, uint16_t flags, struct nvm_ret *ret)
{
	int opt = flags & NVM_CMD_MASK_ADDR;

	opt = opt ? opt : (dev->cmd_opts & NVM_CMD_MASK_ADDR);

	switch(opt) {
	case NVM_CMD_SCALAR:
		if (meta) {
			errno = EINVAL;
			return -1;
		}

		return dev->be->scalar_erase(dev, addrs, naddrs, flags, ret);
	case NVM_CMD_VECTOR:
		return dev->be->vector_erase(dev, addrs, naddrs, meta, flags,
					     ret);
	default:
		errno = EINVAL;
		return -1;
	}
}

int nvm_cmd_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  const void *data, const void *meta, uint16_t flags,
		  struct nvm_ret *ret)
{
	int opt = flags & NVM_CMD_MASK_ADDR;

	opt = opt ? opt : (dev->cmd_opts & NVM_CMD_MASK_ADDR);

	switch(opt) {
	case NVM_CMD_SCALAR:
		return dev->be->scalar_write(dev, *addrs, naddrs, data, meta,
					     flags, ret);
	case NVM_CMD_VECTOR:
		return dev->be->vector_write(dev, addrs, naddrs, data, meta,
					     flags, ret);
	default:
		errno = EINVAL;
		return -1;
	}
}

int nvm_cmd_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 void *data, void *meta, uint16_t flags,
		 struct nvm_ret *ret)
{
	int opt = flags & NVM_CMD_MASK_ADDR;

	opt = opt ? opt : (dev->cmd_opts & NVM_CMD_MASK_ADDR);

	switch(opt) {
	case NVM_CMD_SCALAR:
		return dev->be->scalar_read(dev, *addrs, naddrs, data, meta,
					    flags, ret);
	case NVM_CMD_VECTOR:
		return dev->be->vector_read(dev, addrs, naddrs, data, meta,
					    flags, ret);
	default:
		errno = EINVAL;
		return -1;
	}
}

int nvm_cmd_copy(struct nvm_dev *dev, struct nvm_addr src[],
		 struct nvm_addr dst[], int naddrs, uint16_t flags,
		 struct nvm_ret *ret)
{
	return dev->be->vector_copy(dev, src, dst, naddrs, flags, ret);
}
