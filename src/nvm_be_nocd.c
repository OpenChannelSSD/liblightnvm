/*
 * be_nocd - Backend enabling Non Open-Channel Devices with the liblightnvm API
 *
 * Copyright (C) Simon A. F. Lund <slund@cnexlabs.com>
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

#ifndef NVM_BE_SPDK_ENABLED
struct nvm_be nvm_be_nocd = {
	.id = NVM_BE_NOCD,
	.name = "NVM_BE_NOCD",

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.async_init = nvm_be_nosys_async_init,
	.async_term = nvm_be_nosys_async_term,
	.async_poke = nvm_be_nosys_async_poke,
	.async_wait = nvm_be_nosys_async_wait,

	.idfy = nvm_be_nosys_idfy,
	.rprt = nvm_be_nosys_rprt,
	.gfeat = nvm_be_nosys_gfeat,
	.sfeat = nvm_be_nosys_sfeat,
	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.scalar_erase = nvm_be_nosys_scalar_erase,
	.scalar_write = nvm_be_nosys_scalar_write,
	.scalar_read = nvm_be_nosys_scalar_read,

	.vector_erase = nvm_be_nosys_vector_erase,
	.vector_write = nvm_be_nosys_vector_write,
	.vector_read = nvm_be_nosys_vector_read,
	.vector_copy = nvm_be_nosys_vector_copy,
};
#else

#include <nvm_async.h>
#include <nvm_dev.h>
#include <nvm_cmd.h>
#include <nvm_sgl.h>
#include <nvm_be.h>
#include <nvm_be_nocd.h>

void nvm_be_nocd_close(struct nvm_dev *dev)
{
	nvm_be_spdk_close(dev);
}

static struct nvm_be_nocd_state *nvm_be_nocd_reinit(struct nvm_dev *dev)
{
	struct nvm_be_spdk_state *spdk = dev->be_state;
	struct nvm_be_nocd_state *nocd = NULL;
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	const size_t ndescr = geo->l.npugrp * geo->l.npunit * geo->l.nchunk;
	const size_t nbytes = ndescr * sizeof(struct nvm_spec_rprt_descr);

	if (!(spdk && ndescr && nbytes)) {
		NVM_DEBUG("spdk: %p, ndescr: %zu, nbytes; %zu",
			  (void*)spdk, ndescr, nbytes);
		errno = EINVAL;
		return NULL;
	}

	nocd = realloc(spdk, sizeof(*nocd) + nbytes);
	if (!nocd) {
		NVM_DEBUG("FAILED: realloc spdk: %p, nocd: %p",
			  (void*)spdk, (void*)nocd);
		return NULL;
	}

	nocd->ndescr = ndescr;

	for (size_t idx = 0; idx < nocd->ndescr; ++idx) {
		const struct nvm_addr addr = {
			.val = 0,
			.l.chunk = idx
		};
		nocd->descr[idx].cs = NVM_CHUNK_STATE_FREE;
		nocd->descr[idx].ct = NVM_CHUNK_TYPE_ARWR;
		nocd->descr[idx].wli = 0;
		nocd->descr[idx].addr = nvm_addr_gen2dev(dev, addr);
		nocd->descr[idx].naddrs = geo->l.nsectr;
		nocd->descr[idx].wp = 0;
	}

	return nocd;
}

struct nvm_dev *nvm_be_nocd_open(const char *dev_path, int flags)
{
	struct nvm_be_nocd_state *nocd = NULL;
	struct nvm_be_spdk_state *spdk = NULL;
	struct nvm_dev *dev = NULL;
	int err;

	spdk = nvm_be_spdk_state_init(dev_path, flags);
	if (!spdk) {
		NVM_DEBUG("FAILED: nvm_be_spdk_state_init");
		free(dev);
		return NULL;
	}
	
	// Device initialization after successful backend initialization
	dev = malloc(sizeof(*dev));
	if (!dev) {
		NVM_DEBUG("FAILED: malloc(nvm_dev)");
		return NULL;
	}
	memset(dev, 0, sizeof(*dev));

	dev->be_state = spdk;
	dev->nsid = spdk->nsid;
	memcpy(&dev->ns, &spdk->nsdata, sizeof(dev->ns));

	err = nvm_be_populate(dev, &nvm_be_nocd);
	if (err) {
		NVM_DEBUG("FAILED: nvm_be_populate, err: %d", err);
		goto failed;
	}

	nocd = nvm_be_nocd_reinit(dev);
	if (!nocd) {
		NVM_DEBUG("FAILED: nvm_be_nocd_reinit, err: %d", err);
		goto failed;
	}

	dev->be_state = nocd;

	NVM_DEBUG("INFO: NVM_BE_NOCD is live!");

	return dev;

failed:
	nvm_be_nocd_close(dev);
	free(dev);
	return NULL;
}

struct nvm_spec_idfy *nvm_be_nocd_idfy(struct nvm_dev *dev,
					struct nvm_ret *NVM_UNUSED(ret))
{
	struct nvm_spec_idfy *idfy = NULL;
	const size_t idfy_len = sizeof(*idfy);

	const size_t lba_byts = 1 << dev->ns.lbaf[dev->ns.flbas & 0xf].ds;
	const size_t ncap_byts = dev->ns.ncap * lba_byts;

	idfy = nvm_buf_alloc(dev, idfy_len, NULL);
	if (!idfy) {
		NVM_DEBUG("nvm_buf_alloc: FAILED");
		errno = ENOMEM;
		return NULL;
	}
	memset(idfy, 0, sizeof(*idfy));

	idfy->s.verid = NVM_SPEC_VERID_20;

	idfy->s20.mccap = 0x0;
	idfy->s20.wit = 0x0;

	idfy->s20.lgeo.npugrp = 1;
	idfy->s20.lgeo.npunit = 1;
	idfy->s20.lgeo.nsectr = 4096;
	idfy->s20.lgeo.nchunk = ncap_byts / (idfy->s20.lgeo.nsectr * lba_byts);

	idfy->s20.lbaf.pugrp = 1;
	idfy->s20.lbaf.punit = 1;
	idfy->s20.lbaf.chunk = 16;
	idfy->s20.lbaf.sectr = 12;

	idfy->s20.wrt.ws_min = 1;
	idfy->s20.wrt.ws_opt = 4;
	idfy->s20.wrt.mw_cunits = 0;
	idfy->s20.wrt.maxoc = 0;
	idfy->s20.wrt.maxocpu = 0;

	idfy->s20.perf.trdt = 1;
	idfy->s20.perf.trdm = 1;
	idfy->s20.perf.twrt = 1;
	idfy->s20.perf.twrm = 1;
	idfy->s20.perf.tcet = 1;
	idfy->s20.perf.tcem = 1;

	return idfy;
}

int nvm_be_nocd_gfeat(struct nvm_dev *dev, uint8_t id,
		       union nvm_nvme_feat *feat,
		       struct nvm_ret *ret)
{
	return nvm_be_spdk_gfeat(dev, id, feat, ret);
}

int nvm_be_nocd_sfeat(struct nvm_dev *dev, uint8_t id,
		       const union nvm_nvme_feat *feat,
		       struct nvm_ret *ret)
{
	return nvm_be_spdk_sfeat(dev, id, feat, ret);
}

struct nvm_spec_rprt *nvm_be_nocd_rprt(struct nvm_dev *dev,
					struct nvm_addr *addr,
					int NVM_UNUSED(opt),
					struct nvm_ret *NVM_UNUSED(ret))
{
	const size_t DESCR_NBYTES = sizeof(struct nvm_spec_rprt_descr);
	struct nvm_be_nocd_state *state = dev->be_state;
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	size_t lpo = addr ? nvm_addr_gen2lpo(dev, *addr) : 0;
	size_t idx = addr ? lpo / sizeof(struct nvm_spec_rprt_descr) : 0;
	struct nvm_spec_rprt *rprt = NULL;
	size_t rprt_len, ndescr;

	if (NVM_SPEC_VERID_20 != dev->verid) {
		errno = EINVAL;
		return NULL;
	}

	ndescr = addr ? geo->l.nchunk : geo->l.nchunk * geo->l.npunit * geo->l.npugrp;
	rprt_len = ndescr * DESCR_NBYTES + sizeof(rprt->ndescr);

	rprt = nvm_buf_alloc(dev, rprt_len, NULL);
	if (!rprt) {
		errno = ENOMEM;
		return NULL;
	}
	memset(rprt, 0, rprt_len);
	rprt->ndescr = ndescr;

	memcpy(rprt->descr, state->descr + idx, ndescr * DESCR_NBYTES);

	return rprt;
}

int nvm_be_nocd_vector_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, void *meta, uint16_t flags,
			      struct nvm_ret *ret)
{
	if (meta) {
		NVM_DEBUG("FAILED: erase-meta is not supported");
		errno = ENOSYS;
		return -1;
	}

	for (int i = 0; i < naddrs; ++i) {
		int err;

		err = nvm_be_spdk_scalar_erase(dev, addrs, naddrs, flags, ret);
		if (err) {
			NVM_DEBUG("FAILED: nvm_be_nocd_scalar_erase, err: %d",
				  err);
			return err;
		}
	}

	return 0;
}

int nvm_be_nocd_vector_write(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, const void *data, const void *meta,
			      uint16_t flags, struct nvm_ret *ret)
{
	int contig = 1;

	// Map contiguous vector-command to scalar-command
	for (int i = 1; (i < naddrs) && contig; ++i) {
		contig = addrs[i].l.sectr == addrs[i-1].l.sectr + 1;
	}
	if (contig) {
		return nvm_be_spdk_scalar_write(dev, addrs[0], naddrs, data,
						meta, flags, ret);
	}

	if (flags & NVM_CMD_ASYNC) {
		NVM_DEBUG("FAILED: cmd-split/NVM_CMD_ASYNC is not supported");
		errno = ENOSYS;
		return -1;
	}

	// Mimic vector-IO with multiple scalar-IO commands
	{
		const struct nvm_geo *geo = nvm_dev_get_geo(dev);
		const size_t dskip = geo->l.nbytes;
		const size_t mskip = geo->l.nbytes_oob;
		const char *cdata = data;
		const char *cmeta = meta;

		for (int i = 0; i < naddrs; ++i) {
			int err;

			err = nvm_be_spdk_scalar_write(dev, addrs[i], 1, data,
						       meta, flags, ret);
			if (err) {
				return err;
			}

			cdata = cdata ? cdata + dskip : cdata;
			cmeta = cmeta ? cmeta + mskip : cmeta;
		}
	}

	return 0;
}

int nvm_be_nocd_vector_read(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *data, void *meta, uint16_t flags,
			     struct nvm_ret *ret)
{
	int contig = 1;

	// Map contiguous vector-command to scalar-command
	for (int i = 1; (i < naddrs) && contig; ++i) {
		contig = addrs[i].l.sectr == addrs[i-1].l.sectr + 1;
	}
	if (contig) {
		return nvm_be_spdk_scalar_read(dev, addrs[0], naddrs, data,
					       meta, flags, ret);
	}

	if (flags & NVM_CMD_ASYNC) {
		NVM_DEBUG("FAILED: cmd-split/NVM_CMD_ASYNC is not supported");
		errno = ENOSYS;
		return -1;
	}

	// Mimic vector-IO with multiple scalar-IO commands
	{
		const struct nvm_geo *geo = nvm_dev_get_geo(dev);
		const size_t dskip = geo->l.nbytes;
		const size_t mskip = geo->l.nbytes_oob;
		char *cdata = data;
		char *cmeta = meta;

		for (int i = 0; i < naddrs; ++i) {
			int err;

			err = nvm_be_spdk_scalar_read(dev, addrs[i], 1, data,
						      meta, flags, ret);
			if (err) {
				return err;
			}

			cdata = cdata ? cdata + dskip : cdata;
			cmeta = cmeta ? cmeta + mskip : cmeta;
		}
	}

	return 0;
}

struct nvm_be nvm_be_nocd = {
	.id = NVM_BE_NOCD,
	.name = "NVM_BE_NOCD",

	.open = nvm_be_nocd_open,
	.close = nvm_be_nocd_close,

	.pass = nvm_be_spdk_pass,

	.async_init = nvm_be_spdk_async_init,
	.async_term = nvm_be_spdk_async_term,
	.async_poke = nvm_be_spdk_async_poke,
	.async_wait = nvm_be_spdk_async_wait,

	.idfy = nvm_be_nocd_idfy,
	.rprt = nvm_be_nocd_rprt,
	.gfeat = nvm_be_nocd_gfeat,
	.sfeat = nvm_be_nocd_sfeat,

	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.scalar_erase = nvm_be_spdk_scalar_erase,
	.scalar_write = nvm_be_spdk_scalar_write,
	.scalar_read = nvm_be_spdk_scalar_read,

	.vector_erase = nvm_be_nocd_vector_erase,
	.vector_write = nvm_be_nocd_vector_write,
	.vector_read = nvm_be_nocd_vector_read,

	.vector_copy = nvm_be_spdk_vector_copy,
};
#endif
