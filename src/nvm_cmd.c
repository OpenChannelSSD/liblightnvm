/*
 * cmd - Encapsulation of command construction and execution
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
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

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
			free(rprt);
			return -1;
		}

		for (des_idx = 0; des_idx < rprt->ndescr; ++des_idx) {
			int des_cur = (des_idx + arb) % rprt->ndescr;
			
			if (rprt->descr[des_cur].chunk_state != cs)
				continue;

			addrs[idx].val = addr.val;
			addrs[idx].l.chunk = des_cur;
			break;
		}

		if (des_idx == rprt->ndescr) {			// No chunk !
			free(rprt);
			return -1;
		}

		free(rprt);
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

int nvm_cmd_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  uint16_t flags, struct nvm_ret *ret)
{
	return dev->be->erase(dev, addrs, naddrs, flags, ret);
}

int nvm_cmd_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *data, void *meta, uint16_t flags,
		  struct nvm_ret *ret)
{
	return dev->be->write(dev, addrs, naddrs, data, meta, flags, ret);
}

int nvm_cmd_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 void *data, void *meta, uint16_t flags,
		 struct nvm_ret *ret)
{
	return dev->be->read(dev, addrs, naddrs, data, meta, flags, ret);
}

int nvm_cmd_copy(struct nvm_dev *dev, struct nvm_addr src[],
		 struct nvm_addr dst[], int naddrs, uint16_t flags,
		 struct nvm_ret *ret)
{
	return dev->be->copy(dev, src, dst, naddrs, flags, ret);
}

