/*
 * vblock - Virtual block functions
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
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_dev.h>
#include <nvm_vblk.h>
#include <nvm_omp.h>

#define NVM_VBLK_CMD_OPTS (NVM_CMD_SYNC | NVM_CMD_VECTOR | NVM_CMD_PRP)

int nvm_vblk_set_async(struct nvm_vblk *vblk, uint32_t depth)
{
	vblk->flags &= ~NVM_CMD_SYNC;
	vblk->flags |= NVM_CMD_ASYNC;

	if (!vblk->async_ctx) {
		if (NULL == (vblk->async_ctx = nvm_async_init(vblk->dev, depth, 0))) {
			NVM_DEBUG("FAILED: nvm_async_init");
			return -1;
		}

		if (depth == 0) {
			depth = nvm_async_get_depth(vblk->async_ctx);
		}

		vblk->rets = calloc(depth, sizeof(struct nvm_ret *));
		for (uint32_t i = 0; i < depth; i++)
			vblk->rets[i] = calloc(1, sizeof(struct nvm_ret));

	}

	return 0;
}

int nvm_vblk_set_scalar(struct nvm_vblk *vblk)
{
	vblk->flags &= ~NVM_CMD_VECTOR;
	vblk->flags |= NVM_CMD_SCALAR;

	return 0;
}

static void vblk_async_callback(struct nvm_ret *ret, void *opaque)
{
	struct nvm_vblk_async_cb_state *state = opaque;
	struct nvm_vblk *vblk = state->vblk;

	if (ret->status) {
		(*state->nerr)++;
	}

	memset(ret, 0, sizeof(*ret));
	vblk->rets[--(vblk->retsp)] = ret;
}

struct nvm_vblk* nvm_vblk_alloc(struct nvm_dev *dev, struct nvm_addr addrs[],
				int naddrs)
{
	struct nvm_vblk *vblk;
	const struct nvm_geo *geo;

	if (naddrs > 128) {
		errno = EINVAL;
		return NULL;
	}

	geo = nvm_dev_get_geo(dev);
	if (!geo) {
		errno = EINVAL;
		return NULL;
	}

	vblk = calloc(1, sizeof(*vblk));
	if (!vblk) {
		errno = ENOMEM;
		return NULL;
	}

	for (int i = 0; i < naddrs; ++i) {
		if (nvm_addr_check(addrs[i], dev)) {
			NVM_DEBUG("FAILED: nvm_addr_check");
			errno = EINVAL;
			free(vblk);
			return NULL;
		}

		vblk->blks[i].ppa = addrs[i].ppa;
	}

	vblk->nblks = naddrs;
	vblk->dev = dev;
	vblk->pos_write = 0;
	vblk->pos_read = 0;
	vblk->flags = NVM_VBLK_CMD_OPTS;

	switch (nvm_dev_get_verid(dev)) {
	case NVM_SPEC_VERID_12:
		vblk->nbytes = vblk->nblks * geo->nplanes * geo->npages *
			       geo->nsectors * geo->sector_nbytes;
		break;

	case NVM_SPEC_VERID_20:
		vblk->nbytes = vblk->nblks * geo->l.nsectr * geo->l.nbytes;
		break;

	default:
		NVM_DEBUG("FAILED: unsupported verid");
		errno = ENOSYS;
		free(vblk);
		return NULL;
	}

	return vblk;
}

struct nvm_vblk *nvm_vblk_alloc_line(struct nvm_dev *dev, int ch_bgn,
				     int ch_end, int lun_bgn, int lun_end,
				     int blk)
{
	const int verid = nvm_dev_get_verid(dev);
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	struct nvm_vblk *vblk;

	vblk = nvm_vblk_alloc(dev, NULL, 0);
	if (!vblk)
		return NULL;	// Propagate errno

	switch (verid) {
	case NVM_SPEC_VERID_12:
		for (int lun = lun_bgn; lun <= lun_end; ++lun) {
			for (int ch = ch_bgn; ch <= ch_end; ++ch) {
				vblk->blks[vblk->nblks].ppa = 0;
				vblk->blks[vblk->nblks].g.ch = ch;
				vblk->blks[vblk->nblks].g.lun = lun;
				vblk->blks[vblk->nblks].g.blk = blk;
				++(vblk->nblks);
			}
		}

		vblk->nbytes = vblk->nblks * geo->nplanes * geo->npages *
			       geo->nsectors * geo->sector_nbytes;
		break;

	case NVM_SPEC_VERID_20:
		for (int punit = lun_bgn; punit <= lun_end; ++punit) {
			for (int pugrp = ch_bgn; pugrp <= ch_end; ++pugrp) {
				vblk->blks[vblk->nblks].ppa = 0;
				vblk->blks[vblk->nblks].l.pugrp = pugrp;
				vblk->blks[vblk->nblks].l.punit = punit;
				vblk->blks[vblk->nblks].l.chunk = blk;
				++(vblk->nblks);
			}
		}

		vblk->nbytes = vblk->nblks * geo->l.nsectr * geo->l.nbytes;
		break;

	default:
		NVM_DEBUG("FAILED: unsupported verid: %d", verid);
		nvm_buf_free(vblk);
		errno = ENOSYS;
		return NULL;
	}

	for (int i = 0; i < vblk->nblks; ++i) {
		if (nvm_addr_check(vblk->blks[i], dev)) {
			NVM_DEBUG("FAILED: nvm_addr_check");
			free(vblk);
			errno = EINVAL;
			return NULL;
		}
	}

	return vblk;
}

void nvm_vblk_free(struct nvm_vblk *vblk)
{
	free(vblk);
}

static inline int cmd_nblks(int nblks, int cmd_nblks_max)
{
	int count = cmd_nblks_max;

	while (nblks % count && count > 1) --count;

	return count;
}

static inline ssize_t vblk_erase_s12(struct nvm_vblk *vblk)
{
	size_t nerr = 0;
	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const int BLK_NADDRS = geo->nplanes;
	const int CMD_NBLKS = cmd_nblks(vblk->nblks,
			nvm_dev_get_erase_naddrs_max(vblk->dev) / BLK_NADDRS);

	const int pmode = nvm_dev_get_pmode(vblk->dev);

	for (int off = 0; off < vblk->nblks; off += CMD_NBLKS) {
		ssize_t err;
		struct nvm_ret ret = { 0 };

		const int nblks = NVM_MIN(CMD_NBLKS, vblk->nblks - off);
		const int naddrs = nblks * BLK_NADDRS;

		struct nvm_addr addrs[naddrs];

		for (int i = 0; i < naddrs; ++i) {
			const int idx = off + (i / BLK_NADDRS);

			addrs[i].ppa = vblk->blks[idx].ppa;
			addrs[i].g.pl = i % geo->nplanes;
		}

		err = nvm_cmd_erase(vblk->dev, addrs, naddrs, NULL,
				    pmode | NVM_VBLK_CMD_OPTS, &ret);
		if (err)
			++nerr;
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	vblk->pos_write = 0;
	vblk->pos_read = 0;

	return vblk->nbytes;
}

static inline ssize_t vblk_erase_s20(struct nvm_vblk *vblk)
{
	size_t nerr = 0;

	const int CMD_NBLKS = cmd_nblks(vblk->nblks,
				nvm_dev_get_erase_naddrs_max(vblk->dev));

	for (int off = 0; off < vblk->nblks; off += CMD_NBLKS) {
		const int naddrs = NVM_MIN(CMD_NBLKS, vblk->nblks - off);
		struct nvm_ret ret = { 0 };

		struct nvm_addr addrs[naddrs];

		for (int i = 0; i < naddrs; ++i)
			addrs[i].ppa = vblk->blks[off + i].ppa;

		if (nvm_cmd_erase(vblk->dev, addrs, naddrs, NULL, 0x0, &ret))
			++nerr;
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	vblk->pos_write = 0;
	vblk->pos_read = 0;

	return vblk->nbytes;
}

ssize_t nvm_vblk_erase(struct nvm_vblk *vblk)
{
	const int verid = nvm_dev_get_verid(nvm_vblk_get_dev(vblk));

	switch (verid) {
	case NVM_SPEC_VERID_12:
		return vblk_erase_s12(vblk);

	case NVM_SPEC_VERID_20:
		return vblk_erase_s20(vblk);

	default:
		NVM_DEBUG("FAILED: unsupported verid: %d", verid);
		errno = ENOSYS;
		return -1;
	}
}

static inline int _cmd_nspages(int nblks, int cmd_nspages_max)
{
	int cmd_nspages = cmd_nspages_max;

	while(nblks % cmd_nspages && cmd_nspages > 1) --cmd_nspages;

	return cmd_nspages;
}

static inline int _vblk_async_greedy_reap(struct nvm_vblk *vblk)
{
	int r, nevents = 0;

	// spin until something can be reaped
	do {
		if (-1 == (nevents = nvm_async_poke(vblk->dev, vblk->async_ctx, 0)))
			return -1;
	} while (nevents == 0);

	// reap until empty
	do {
		if (-1 == (r = nvm_async_poke(vblk->dev, vblk->async_ctx, 0)))
			return -1;

		nevents += r;
	} while (r != 0);

	return nevents;
}

static inline int vblk_io_async(struct nvm_vblk *vblk, const size_t vsectr_bgn,
	const size_t count, void *buf, void *meta_buf, char *pad_buf,
	int write)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const size_t sectr_nbytes = geo->l.nbytes;
	const size_t nsectrs = count / sectr_nbytes;
	const size_t stripe_nsectrs = nvm_dev_get_ws_opt(vblk->dev);
	const size_t nstripes = nsectrs / stripe_nsectrs;

	int err;
	uint64_t nerr = 0;

	struct nvm_vblk_async_cb_state state = {
		.nerr = &nerr,
		.vblk = vblk,
	};

	size_t cnk_bgn = (vsectr_bgn / stripe_nsectrs) % vblk->nblks;
	NVM_DEBUG("cnk_bgn: %ld", cnk_bgn);
	for (size_t stripe = 0; stripe < nstripes; stripe++) {
		size_t cnk_idx = (cnk_bgn + stripe) % vblk->nblks;
		size_t cnk_off = (stripe / vblk->nblks) * stripe_nsectrs;

		char *bufp = pad_buf ? pad_buf :
			(char *)buf + (sectr_nbytes * stripe_nsectrs * stripe);

		struct nvm_addr addrs[stripe_nsectrs];
		for (size_t i = 0; i < stripe_nsectrs; i++) {
			addrs[i].val = vblk->blks[cnk_idx].val;
			addrs[i].l.sectr = cnk_off + i;
		}

		// this basically makes sure we never hit an EAGAIN below in
		// the nvm_cmd_read/write call.
		if (vblk->retsp == (nvm_async_get_depth(vblk->async_ctx) - 1)) {
			if (_vblk_async_greedy_reap(vblk) < 0) {
				NVM_DEBUG("FAILED: _vblk_async_greedy_reap: %d", errno);
				return -1;
			}
		}

		struct nvm_ret *ret = vblk->rets[vblk->retsp++];
		if (!ret) {
			NVM_DEBUG("should not happen; retsp = %d", vblk->retsp);
			errno = ENOMEM;
			return -1;
		}

		ret->async.ctx = vblk->async_ctx;
		ret->async.cb = vblk_async_callback;
		ret->async.cb_arg = &state;

		while(1) {
			err = write ?
				nvm_cmd_write(vblk->dev, addrs, stripe_nsectrs,
					      bufp, meta_buf, vblk->flags,
					      ret) :
				nvm_cmd_read(vblk->dev, addrs, stripe_nsectrs,
					     bufp, meta_buf, vblk->flags,
					     ret);

			if (err < 0) {
				if (errno == EAGAIN) {
					if (_vblk_async_greedy_reap(vblk) < 0) {
						NVM_DEBUG("FAILED: _vblk_async_greedy_reap: %d", errno);
						return -1;
					}

					continue;
				}

				// propagate errno
				return -1;
			}

			break;
		}


		if (((stripe + 1) % vblk->nblks) == 0) {
			if (nvm_async_wait(vblk->dev, vblk->async_ctx) < 0) {
				NVM_DEBUG("FAILED: nvm_async_wait");
				return -1;
			}
		}
	}

	err = nvm_async_wait(vblk->dev, vblk->async_ctx);
	if (err < 0) {
		return -1;
	}

	return nerr;
}

static inline ssize_t vblk_async_pwrite_s20(struct nvm_vblk *vblk,
					    const void *buf,
					    size_t count, size_t offset)
{
	size_t nerr = 0;

	const uint32_t WS_OPT = nvm_dev_get_ws_opt(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const size_t sectr_nbytes = geo->l.nbytes;
	const size_t nsectr = count / sectr_nbytes;

	const size_t vsectr_bgn = offset / sectr_nbytes;

	const size_t cmd_nsectr = WS_OPT;

	const size_t meta_tbytes = cmd_nsectr * geo->l.nbytes_oob;
	char *meta_buf = NULL;

	const size_t pad_nbytes = cmd_nsectr * nsectr * geo->l.nbytes;
	char *pad_buf = NULL;

	const int meta_mode = nvm_dev_get_meta_mode(vblk->dev);

	if (nsectr % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned nsectr: %zu", nsectr);
		errno = EINVAL;
		return -1;
	}
	if (vsectr_bgn % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned vsectr_bgn: %zu", vsectr_bgn);
		errno = EINVAL;
		return -1;
	}

	if (!buf) {	// Allocate and use a padding buffer
		pad_buf = nvm_buf_alloc(geo, pad_nbytes);
		if (!pad_buf) {
			NVM_DEBUG("FAILED: nvm_buf_alloc(pad)");
			errno = ENOMEM;
			return -1;
		}

		nvm_buf_fill(pad_buf, pad_nbytes);
	}

	if (meta_mode != NVM_META_MODE_NONE) {	// Meta
		meta_buf = nvm_buf_alloc(geo, meta_tbytes);	// Alloc buf
		if (!meta_buf) {
			nvm_buf_free(pad_buf);
			NVM_DEBUG("FAILED: nvm_buf_alloc(meta)");
			errno = ENOMEM;
			return -1;
		}

		switch(meta_mode) {			// Fill it
			case NVM_META_MODE_ALPHA:
				nvm_buf_fill(meta_buf, meta_tbytes);
				break;
			case NVM_META_MODE_CONST:
				for (size_t i = 0; i < meta_tbytes; ++i)
					meta_buf[i] = 65 + (meta_tbytes % 20);
				break;
			case NVM_META_MODE_NONE:
				break;
		}
	}

	nerr = vblk_io_async(vblk, vsectr_bgn, count, (void *) buf, meta_buf,
			     pad_buf, 1 /* write */);

	nvm_buf_free(pad_buf);
	nvm_buf_free(meta_buf);

	if (nerr) {
		NVM_DEBUG("FAILED: nvm_cmd_write, nerr(%zu)", nerr);
		errno = EIO;
		return -1;
	}

	return count;
}

static inline ssize_t vblk_async_pread_s20(struct nvm_vblk *vblk, void *buf,
					   size_t count, size_t offset)
{
	size_t nerr = 0;

	const uint32_t WS_OPT = nvm_dev_get_ws_opt(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const size_t sectr_nbytes = geo->l.nbytes;
	const size_t nsectr = count / sectr_nbytes;

	const size_t vsectr_bgn = offset / sectr_nbytes;

	if (nsectr % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned nsectr: %zu", nsectr);
		errno = EINVAL;
		return -1;
	}
	if (vsectr_bgn % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned vsectr_bgn: %zu", vsectr_bgn);
		errno = EINVAL;
		return -1;
	}

	nerr = vblk_io_async(vblk, vsectr_bgn, count, buf, NULL, NULL,
			     0 /* write */);
	if (nerr) {
		NVM_DEBUG("FAILED: nvm_cmd_read, nerr(%zu)", nerr);
		errno = EIO;
		return -1;
	}

	return count;
}

static inline ssize_t vblk_sync_pread_s20(struct nvm_vblk *vblk, void *buf,
					  size_t count, size_t offset)
{
	size_t nerr = 0;

	const uint32_t WS_OPT = nvm_dev_get_ws_opt(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);
	const size_t nchunks = vblk->nblks;

	const size_t sectr_nbytes = geo->l.nbytes;
	const size_t nsectr = count / sectr_nbytes;

	const size_t sectr_bgn = offset / sectr_nbytes;
	const size_t sectr_end = sectr_bgn + (count / sectr_nbytes) - 1;

	const size_t cmd_nsectr = vblk->flags & NVM_CMD_VECTOR ? NVM_NADDR_MAX : WS_OPT;

	const int NTHREADS = NVM_MIN(nchunks, nsectr / WS_OPT);

	if (nsectr % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned nsectr: %zu", nsectr);
		errno = EINVAL;
		return -1;
	}
	if (sectr_bgn % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned sectr_bgn: %zu", sectr_bgn);
		errno = EINVAL;
		return -1;
	}

	const int VBLK_FLAGS = vblk->flags;

	#pragma omp parallel for num_threads(NTHREADS) schedule(static,1) reduction(+:nerr) ordered if(NTHREADS>1)
	for (size_t sectr_ofz = sectr_bgn; sectr_ofz <= sectr_end; sectr_ofz += cmd_nsectr) {
		struct nvm_addr addrs[cmd_nsectr];
		char *buf_off = (char*)buf + (sectr_ofz - sectr_bgn) * sectr_nbytes;

		for (size_t idx = 0; idx < cmd_nsectr; ++idx) {
			const size_t sectr = sectr_ofz + idx;
			const size_t wunit = sectr / WS_OPT;
			const size_t rnd = wunit / nchunks;

			const size_t chunk = wunit % nchunks;
			const size_t chunk_sectr = sectr % WS_OPT + rnd * WS_OPT;

			addrs[idx].val = vblk->blks[chunk].val;
			addrs[idx].l.sectr = chunk_sectr;

			if (VBLK_FLAGS & NVM_CMD_SCALAR) break;
		}

		const ssize_t err = nvm_cmd_read(vblk->dev, addrs, cmd_nsectr,
						 buf_off, NULL,
						 VBLK_FLAGS, NULL);
		if (err)
			++nerr;
	}

	if (nerr) {
		NVM_DEBUG("FAILED: nvm_cmd_read, nerr(%zu)", nerr);
		errno = EIO;
		return -1;
	}

	return count;
}

static inline ssize_t vblk_sync_pwrite_s20(struct nvm_vblk *vblk,
					   const void *buf, size_t count,
					   size_t offset)
{
	size_t nerr = 0;

	const uint32_t WS_OPT = nvm_dev_get_ws_opt(vblk->dev);

	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);
	const size_t nchunks = vblk->nblks;

	const size_t sectr_nbytes = geo->l.nbytes;
	const size_t nsectr = count / sectr_nbytes;

	const size_t sectr_bgn = offset / sectr_nbytes;
	const size_t sectr_end = sectr_bgn + (count / sectr_nbytes) - 1;

	const size_t cmd_nsectr = WS_OPT;

	const size_t meta_tbytes = cmd_nsectr * geo->l.nbytes_oob;
	char *meta_buf = NULL;

	const size_t pad_nbytes = cmd_nsectr * nsectr * geo->l.nbytes;
	char *pad_buf = NULL;

	const int NTHREADS = NVM_MIN(nchunks, nsectr / WS_OPT);

	const int meta_mode = nvm_dev_get_meta_mode(vblk->dev);

	if (nsectr % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned nsectr: %zu", nsectr);
		errno = EINVAL;
		return -1;
	}
	if (sectr_bgn % WS_OPT) {
		NVM_DEBUG("FAILED: unaligned sectr_bgn: %zu", sectr_bgn);
		errno = EINVAL;
		return -1;
	}

	if (!buf) {	// Allocate and use a padding buffer
		pad_buf = nvm_buf_alloc(geo, pad_nbytes);
		if (!pad_buf) {
			NVM_DEBUG("FAILED: nvm_buf_alloc(pad)");
			errno = ENOMEM;
			return -1;
		}

		nvm_buf_fill(pad_buf, pad_nbytes);
	}

	if (meta_mode != NVM_META_MODE_NONE) {	// Meta
		meta_buf = nvm_buf_alloc(geo, meta_tbytes);	// Alloc buf
		if (!meta_buf) {
			nvm_buf_free(pad_buf);
			NVM_DEBUG("FAILED: nvm_buf_alloc(meta)");
			errno = ENOMEM;
			return -1;
		}

		switch(meta_mode) {			// Fill it
			case NVM_META_MODE_ALPHA:
				nvm_buf_fill(meta_buf, meta_tbytes);
				break;
			case NVM_META_MODE_CONST:
				for (size_t i = 0; i < meta_tbytes; ++i)
					meta_buf[i] = 65 + (meta_tbytes % 20);
				break;
			case NVM_META_MODE_NONE:
				break;
		}
	}

	const int VBLK_FLAGS = vblk->flags;

	#pragma omp parallel for num_threads(NTHREADS) schedule(static,1) reduction(+:nerr) ordered if(NTHREADS>1)
	for (size_t sectr_ofz = sectr_bgn; sectr_ofz <= sectr_end; sectr_ofz += cmd_nsectr) {
		struct nvm_ret ret = { 0 };

		struct nvm_addr addrs[cmd_nsectr];
		char *buf_off;

		if (pad_buf)
			buf_off = pad_buf;
		else
			buf_off = (char*)buf + (sectr_ofz - sectr_bgn) * sectr_nbytes;

		for (size_t idx = 0; idx < cmd_nsectr; ++idx) {
			const size_t sectr = sectr_ofz + idx;
			const size_t wunit = sectr / WS_OPT;
			const size_t rnd = wunit / nchunks;

			const size_t chunk = wunit % nchunks;
			const size_t chunk_sectr = sectr % WS_OPT + rnd * WS_OPT;

			addrs[idx].ppa = vblk->blks[chunk].ppa;
			addrs[idx].l.sectr = chunk_sectr;
		}

		const ssize_t err = nvm_cmd_write(vblk->dev, addrs, cmd_nsectr,
						  buf_off, meta_buf,
						  VBLK_FLAGS, &ret);
		if (err)
			++nerr;

		#pragma omp ordered
		{}
	}

	nvm_buf_free(pad_buf);
	nvm_buf_free(meta_buf);

	if (nerr) {
		NVM_DEBUG("FAILED: nvm_cmd_write, nerr(%zu)", nerr);
		errno = EIO;
		return -1;
	}

	return count;
}

static inline ssize_t vblk_pwrite_s12(struct nvm_vblk *vblk, const void *buf,
				      size_t count, size_t offset)
{
	size_t nerr = 0;
	const int PMODE = nvm_dev_get_pmode(vblk->dev);
	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const int SPAGE_NADDRS = geo->nplanes * geo->nsectors;
	const int CMD_NSPAGES = _cmd_nspages(vblk->nblks,
			nvm_dev_get_write_naddrs_max(vblk->dev) / SPAGE_NADDRS);

	const int ALIGN = SPAGE_NADDRS * geo->sector_nbytes;
	const int NTHREADS = vblk->nblks < CMD_NSPAGES ? 1 : vblk->nblks / CMD_NSPAGES;

	const size_t bgn = offset / ALIGN;
	const size_t end = bgn + (count / ALIGN);

	char *padding_buf = NULL;

	const size_t meta_tbytes = CMD_NSPAGES * SPAGE_NADDRS * geo->meta_nbytes;
	char *meta = NULL;

	const int meta_mode = nvm_dev_get_meta_mode(vblk->dev);

	if (offset + count > vblk->nbytes) {		// Check bounds
		errno = EINVAL;
		return -1;
	}

	if ((count % ALIGN) || (offset % ALIGN)) {	// Check align
		errno = EINVAL;
		return -1;
	}

	if (!buf) {	// Allocate and use a padding buffer
		const size_t nbytes = CMD_NSPAGES * SPAGE_NADDRS * geo->sector_nbytes;

		padding_buf = nvm_buf_alloc(geo, nbytes);
		if (!padding_buf) {
			NVM_DEBUG("FAILED: nvm_buf_alloc(padding)");
			errno = ENOMEM;
			return -1;
		}
		nvm_buf_fill(padding_buf, nbytes);
	}

	if (meta_mode != NVM_META_MODE_NONE) {	// Meta
		meta = nvm_buf_alloc(geo, meta_tbytes);		// Alloc buf
		if (!meta) {
			NVM_DEBUG("FAILED: nvm_buf_alloc(meta)");
			errno = ENOMEM;
			return -1;
		}

		switch(meta_mode) {			// Fill it
			case NVM_META_MODE_ALPHA:
				nvm_buf_fill(meta, meta_tbytes);
				break;
			case NVM_META_MODE_CONST:
				for (size_t i = 0; i < meta_tbytes; ++i)
					meta[i] = 65 + (meta_tbytes % 20);
				break;
			case NVM_META_MODE_NONE:
				break;
		}
	}

	#pragma omp parallel for num_threads(NTHREADS) schedule(static,1) reduction(+:nerr) ordered if(NTHREADS>1)
	for (size_t off = bgn; off < end; off += CMD_NSPAGES) {
		struct nvm_ret ret = { 0 };

		const int nspages = NVM_MIN(CMD_NSPAGES, (int)(end - off));
		const int naddrs = nspages * SPAGE_NADDRS;

		struct nvm_addr addrs[naddrs];
		const char *buf_off;

		if (padding_buf)
			buf_off = padding_buf;
		else
			buf_off = (const char*)buf + (off - bgn) * geo->sector_nbytes * SPAGE_NADDRS;

		for (int i = 0; i < naddrs; ++i) {
			const int spg = off + (i / SPAGE_NADDRS);
			const int idx = spg % vblk->nblks;
			const int pg = (spg / vblk->nblks) % geo->npages;

			addrs[i].ppa = vblk->blks[idx].ppa;
			addrs[i].g.pg = pg;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
			addrs[i].g.sec = i % geo->nsectors;
		}

		const ssize_t err = nvm_cmd_write(vblk->dev, addrs, naddrs,
						   buf_off, meta, PMODE, &ret);
		if (err)
			++nerr;

		#pragma omp ordered
		{}
	}

	nvm_buf_free(padding_buf);
	nvm_buf_free(meta);

	if (nerr) {
		errno = EIO;
		return -1;
	}

	return count;
}


ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf, size_t count,
			size_t offset)
{
	const int verid = nvm_dev_get_verid(nvm_vblk_get_dev(vblk));

	switch (verid) {
	case NVM_SPEC_VERID_12:
		return vblk_pwrite_s12(vblk, buf, count, offset);

	case NVM_SPEC_VERID_20:
		if (vblk->flags & NVM_CMD_ASYNC) {
			return vblk_async_pwrite_s20(vblk, buf, count, offset);
		} else {
			return vblk_sync_pwrite_s20(vblk, buf, count, offset);
		}

	default:
		NVM_DEBUG("FAILED: unsupported verid: %d", verid);
		errno = ENOSYS;
		return -1;
	}
}

ssize_t nvm_vblk_write(struct nvm_vblk *vblk, const void *buf, size_t count)
{
	ssize_t nbytes = nvm_vblk_pwrite(vblk, buf, count, vblk->pos_write);

	if (nbytes < 0)
		return nbytes;		// Propagate errno

	vblk->pos_write += nbytes;	// All is good, increment write position

	return nbytes;			// Return number of bytes written
}

ssize_t nvm_vblk_pad(struct nvm_vblk *vblk)
{
	return nvm_vblk_write(vblk, NULL, vblk->nbytes - vblk->pos_write);
}

static inline ssize_t vblk_pread_s12(struct nvm_vblk *vblk, void *buf,
				     size_t count, size_t offset)
{
	size_t nerr = 0;
	const int PMODE = nvm_dev_get_pmode(vblk->dev);
	const struct nvm_geo *geo = nvm_dev_get_geo(vblk->dev);

	const int SPAGE_NADDRS = geo->nplanes * geo->nsectors;
	const int CMD_NSPAGES = _cmd_nspages(vblk->nblks,
			nvm_dev_get_read_naddrs_max(vblk->dev) / SPAGE_NADDRS);

	const int ALIGN = SPAGE_NADDRS * geo->sector_nbytes;
	const int NTHREADS = vblk->nblks < CMD_NSPAGES ? 1 : vblk->nblks / CMD_NSPAGES;

	const size_t bgn = offset / ALIGN;
	const size_t end = bgn + (count / ALIGN);

	if (offset + count > vblk->nbytes) {		// Check bounds
		errno = EINVAL;
		return -1;
	}

	if ((count % ALIGN) || (offset % ALIGN)) {	// Check align
		errno = EINVAL;
		return -1;
	}

	#pragma omp parallel for num_threads(NTHREADS) schedule(static,1) reduction(+:nerr) ordered if(NTHREADS>1)
	for (size_t off = bgn; off < end; off += CMD_NSPAGES) {
		struct nvm_ret ret = { 0 };

		const int nspages = NVM_MIN(CMD_NSPAGES, (int)(end - off));
		const int naddrs = nspages * SPAGE_NADDRS;

		struct nvm_addr addrs[naddrs];
		char *buf_off;

		buf_off = (char*)buf + (off - bgn) * geo->sector_nbytes * SPAGE_NADDRS;

		for (int i = 0; i < naddrs; ++i) {
			const int spg = off + (i / SPAGE_NADDRS);
			const int idx = spg % vblk->nblks;
			const int pg = (spg / vblk->nblks) % geo->npages;

			addrs[i].ppa = vblk->blks[idx].ppa;
			addrs[i].g.pg = pg;
			addrs[i].g.pl = (i / geo->nsectors) % geo->nplanes;
			addrs[i].g.sec = i % geo->nsectors;
		}

		const ssize_t err = nvm_cmd_read(vblk->dev, addrs, naddrs,
						 buf_off, NULL, PMODE, &ret);
		if (err)
			++nerr;

		#pragma omp ordered
		{}
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	return count;
}

ssize_t nvm_vblk_pread(struct nvm_vblk *vblk, void *buf, size_t count,
		       size_t offset)
{
	const int verid = nvm_dev_get_verid(nvm_vblk_get_dev(vblk));

	switch (verid) {
	case NVM_SPEC_VERID_12:
		return vblk_pread_s12(vblk, buf, count, offset);

	case NVM_SPEC_VERID_20:
		if (vblk->flags & NVM_CMD_ASYNC) {
			return vblk_async_pread_s20(vblk, buf, count, offset);
		} else {
			return vblk_sync_pread_s20(vblk, buf, count, offset);
		}

	default:
		NVM_DEBUG("FAILED: unsupported verid: %d", verid);
		errno = ENOSYS;
		return -1;
	}
}

ssize_t nvm_vblk_read(struct nvm_vblk *vblk, void *buf, size_t count)
{
	ssize_t nbytes = nvm_vblk_pread(vblk, buf, count, vblk->pos_read);

	if (nbytes < 0)
		return nbytes;		// Propagate `errno`

	vblk->pos_read += nbytes;	// All is good, increment read position

	return nbytes;			// Return number of bytes read
}

static inline ssize_t vblk_copy_s20(struct nvm_vblk *src, struct nvm_vblk *dst)
{
	size_t nerr = 0;

	const size_t offset = 0;
	const size_t count = src->nbytes;

	const uint32_t WS_MIN = nvm_dev_get_ws_min(src->dev);

	const struct nvm_geo *geo = nvm_dev_get_geo(src->dev);
	const size_t nchunks = src->nblks;

	const size_t sectr_nbytes = geo->l.nbytes;
	const size_t nsectr = count / sectr_nbytes;

	const size_t sectr_bgn = offset / sectr_nbytes;
	const size_t sectr_end = sectr_bgn + (count / sectr_nbytes) - 1;

	const size_t cmd_nsectr_max = (NVM_NADDR_MAX / WS_MIN) * WS_MIN;

	if (nsectr % WS_MIN) {
		NVM_DEBUG("FAILED: unaligned nsectr: %zu", nsectr);
		errno = EINVAL;
		return -1;
	}
	if (sectr_bgn % WS_MIN) {
		NVM_DEBUG("FAILED: unaligned sectr_bgn: %zu", sectr_bgn);
		errno = EINVAL;
		return -1;
	}

	for (size_t sectr_ofz = sectr_bgn; sectr_ofz <= sectr_end; sectr_ofz += cmd_nsectr_max) {
		struct nvm_ret ret = { 0 };

		const size_t cmd_nsectr = NVM_MIN(sectr_end - sectr_ofz + 1, cmd_nsectr_max);

		struct nvm_addr addrs_src[cmd_nsectr];
		struct nvm_addr addrs_dst[cmd_nsectr];

		for (size_t idx = 0; idx < cmd_nsectr; ++idx) {
			const size_t sectr = sectr_ofz + idx;
			const size_t wunit = sectr / WS_MIN;
			const size_t rnd = wunit / nchunks;

			const size_t chunk = wunit % nchunks;
			const size_t chunk_sectr = sectr % WS_MIN + rnd * WS_MIN;

			addrs_src[idx].val = src->blks[chunk].val;
			addrs_src[idx].l.sectr = chunk_sectr;

			addrs_dst[idx].val = dst->blks[chunk].val;
			addrs_dst[idx].l.sectr = chunk_sectr;
		}

		const ssize_t err = nvm_cmd_copy(src->dev, addrs_src,
						 addrs_dst, cmd_nsectr,
						 NVM_VBLK_CMD_OPTS, &ret);
		if (err)
			++nerr;
	}

	if (nerr) {
		NVM_DEBUG("FAILED: nvm_cmd_write, nerr(%zu)", nerr);
		errno = EIO;
		return -1;
	}

	return count;
}

ssize_t nvm_vblk_copy(struct nvm_vblk *src, struct nvm_vblk *dst,
		      int NVM_UNUSED(flags))
{
	const int verid = nvm_dev_get_verid(nvm_vblk_get_dev(src));

	if (src->dev != dst->dev) {
		NVM_DEBUG("FAILED: unsupported cross device copy");
		errno = ENOSYS;
		return -1;
	}

	if (src->nblks != dst->nblks) {
		NVM_DEBUG("FAILED: unbalanced vblks");
		errno = ENOSYS;
		return -1;
	}

	if (src->nbytes != dst->nbytes) {
		NVM_DEBUG("FAILED: unbalanced vblks");
		errno = ENOSYS;
		return -1;
	}

	switch (verid) {
	case NVM_SPEC_VERID_20:
		return vblk_copy_s20(src, dst);

	case NVM_SPEC_VERID_12:
		NVM_DEBUG("FAILED: not implemented, verid: %d", verid);
		errno = ENOSYS;
		return -1;

	default:
		NVM_DEBUG("FAILED: unsupported, verid: %d", verid);
		errno = ENOSYS;
		return -1;
	}
}

struct nvm_addr *nvm_vblk_get_addrs(struct nvm_vblk *vblk)
{
	return vblk->blks;
}

int nvm_vblk_get_naddrs(struct nvm_vblk *vblk)
{
	return vblk->nblks;
}

size_t nvm_vblk_get_nbytes(struct nvm_vblk *vblk)
{
	return vblk->nbytes;
}

size_t nvm_vblk_get_pos_read(struct nvm_vblk *vblk)
{
	return vblk->pos_read;
}

size_t nvm_vblk_get_pos_write(struct nvm_vblk *vblk)
{
	return vblk->pos_write;
}

struct nvm_dev *nvm_vblk_get_dev(struct nvm_vblk *vblk)
{
	if (!vblk)
		return NULL;

	return vblk->dev;
}

int nvm_vblk_set_pos_read(struct nvm_vblk *vblk, size_t pos)
{
	if (pos > vblk->nbytes) {
		errno = EINVAL;
		return -1;
	}

	vblk->pos_read = pos;

	return 0;
}

int nvm_vblk_set_pos_write(struct nvm_vblk *vblk, size_t pos)
{
	if (pos > vblk->nbytes) {
		errno = EINVAL;
		return -1;
	}

	vblk->pos_write = pos;

	return 0;
}

void nvm_vblk_pr(struct nvm_vblk *vblk)
{
	printf("vblk:\n");
	printf("  dev: {pmode: '%s'}\n", nvm_pmode_str(nvm_dev_get_pmode(vblk->dev)));
	printf("  nblks: %"PRIi32"\n", vblk->nblks);
	printf("  nmbytes: %zu\n", vblk->nbytes >> 20);
	printf("  pos_write: %zu\n", vblk->pos_write);
	printf("  pos_read: %zu\n", vblk->pos_read);
	printf("  flags: 0x08%x\n", vblk->flags);
        nvm_addr_prn(vblk->blks, vblk->nblks, vblk->dev);
}
