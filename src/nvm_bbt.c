/*
 * bbt - Bad-block-table functions
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
#include <nvm_spec.h>
#include <nvm_debug.h>

static inline int _bbt_idx(const struct nvm_dev *dev,
			   const struct nvm_addr addr)
{
	return addr.g.ch * dev->geo.nluns + addr.g.lun;
}

static inline int _blk_idx(const struct nvm_dev *dev,
		           const struct nvm_addr addr)
{
	return addr.g.blk * dev->geo.nplanes + addr.g.pl;
}

static inline int _refresh_counters(struct nvm_dev *dev, struct nvm_bbt *bbt)
{
	int nbad = 0;
	int ngbad = 0;
	int ndmrk = 0;
	int nhmrk = 0;

	for (uint64_t i = 0; i < bbt->nblks; ++i) {
		switch (bbt->blks[i]) {
		case NVM_BBT_FREE:
			break;
		case NVM_BBT_BAD:
			++nbad;
			break;
		case NVM_BBT_GBAD:
			++ngbad;
			break;
		case NVM_BBT_DMRK:
			++ndmrk;
			break;
		case NVM_BBT_HMRK:
			++nhmrk;
			break;

		default:
			errno = EINVAL;
			return -1;
		}
	}

	if (dev->verid == NVM_SPEC_VERID_20) {
		nbad = nbad / dev->geo.nplanes;
		ngbad = ngbad / dev->geo.nplanes;
		ndmrk = ndmrk / dev->geo.nplanes;
		nhmrk = nhmrk / dev->geo.nplanes;
	}

	bbt->nbad = nbad;
	bbt->ngbad = ngbad;
	bbt->ndmrk = ndmrk;
	bbt->nhmrk = nhmrk;

	return 0;
}

int nvm_bbt_flush(struct nvm_dev *dev, struct nvm_addr addr,
		  struct nvm_ret *ret)
{
	const struct nvm_bbt *cached;
	struct nvm_spec_bbt *spec;
	size_t bbt_idx;

	if ((!dev) || (nvm_addr_check(addr, &dev->geo))) {
		NVM_DEBUG("FAILED: !dev or nvm_addr_check failed");
		errno = EINVAL;
		return -1;
	}

	bbt_idx = _bbt_idx(dev, addr);

	cached = dev->bbts[bbt_idx];
	if (!cached)
		return 0;			// Nothing to flush

	spec = nvm_cmd_gbbt(dev, addr, ret);
	if (!spec) {
		NVM_DEBUG("FAILED: nvm_cmd_gbbt failed spec");
		return -1;			// Propagate `errno`
	}

	if (cached->nblks != spec->tblks) {
		NVM_DEBUG("FAILED: cached->nblks(%lu) != spec->tblks(%u)",
			  cached->nblks, spec->tblks);
		errno = EINVAL;
		free(spec);
		return -1;
	}
	
	for (uint64_t i = 0; i < cached->nblks; ++i) {	// Update on device
		struct nvm_addr blk_addr;

		if (cached->blks[i] == spec->blk[i])
			continue;		// Ignore same state

		// Convert "i -> (blk, pl)" and submit changed state
		blk_addr.ppa = cached->addr.ppa;
		blk_addr.g.blk = i / dev->geo.nplanes;
		blk_addr.g.pl = i % dev->geo.nplanes;

		if (nvm_cmd_sbbt(dev, &blk_addr, 1, cached->blks[i],
					ret)) {
			NVM_DEBUG("FAILED: nvm_cmd_sbbt");
			free(spec);
			return -1;		// Propagate `errno`
		}
	}

	free(spec);

	/* Deallocate the bbt entry */
	nvm_bbt_free(dev->bbts[bbt_idx]);
	dev->bbts[bbt_idx] = NULL;

	return 0;
}

int nvm_bbt_flush_all(struct nvm_dev *dev, struct nvm_ret *ret)
{
	for (size_t i = 0; i < dev->nbbts; ++i) {
		struct nvm_addr addr;
		int err;

		addr.ppa = 0;
		addr.g.ch = i % dev->geo.nchannels;
		addr.g.lun = (i / dev->geo.nchannels) % dev->geo.nluns;

		err = nvm_bbt_flush(dev, addr, ret);
		if (err) {
			NVM_DEBUG("FAILED: nvm_bbt_flush failed");
			return err;
		}
	}

	return 0;
}

const struct nvm_bbt *nvm_bbt_get(struct nvm_dev *dev, struct nvm_addr addr,
				  struct nvm_ret *ret)
{
	struct nvm_spec_bbt *spec;
	size_t bbt_idx;

	if ((!dev) || (nvm_addr_check(addr, &dev->geo))) {
		NVM_DEBUG("FAILED: invalid input");
		errno = EINVAL;
		return NULL;
	}
	
	bbt_idx = _bbt_idx(dev, addr);

	/* Return bbt from cache */
	if (dev->bbts_cached && dev->bbts[bbt_idx])
		return dev->bbts[bbt_idx];

	/* Allocate bbt entry in managed memory area */
	if (!dev->bbts[bbt_idx]) {
		int nblks = dev->geo.nblocks * dev->geo.nplanes;

		struct nvm_bbt *bbt;

		bbt = malloc(sizeof(*bbt) + sizeof(*bbt->blks) * nblks);
		if (!bbt) {
			NVM_DEBUG("FAILED: malloc of bbt failed");
			errno = ENOMEM;
			return NULL;
		}

		bbt->dev = dev;
		bbt->addr = addr;
		bbt->nblks = nblks;

		dev->bbts[bbt_idx] = bbt;
	}

	/* Update bbt entry in managed memory area with bbt from device */
	spec = nvm_cmd_gbbt(dev, addr, ret);
	if (!spec) {
		free(dev->bbts[bbt_idx]);
		dev->bbts[bbt_idx] = NULL;

		return NULL;
	}

	if (dev->bbts[bbt_idx]->nblks != spec->tblks) {
		free(dev->bbts[bbt_idx]);
		dev->bbts[bbt_idx] = NULL;

		return NULL;
	}

	for (uint64_t i = 0 ; i < dev->bbts[bbt_idx]->nblks; ++i)
		dev->bbts[bbt_idx]->blks[i] = spec->blk[i];

	dev->bbts[bbt_idx]->nbad = spec->tfact;
	dev->bbts[bbt_idx]->ngbad = spec->tgrown;
	dev->bbts[bbt_idx]->ndmrk = spec->tdresv;
	dev->bbts[bbt_idx]->nhmrk = spec->thresv;

	free(spec);

	return dev->bbts[bbt_idx];
}

int nvm_bbt_set(struct nvm_dev *dev, const struct nvm_bbt *bbt,
		struct nvm_ret *ret)
{
	struct nvm_addr addr = bbt->addr;
	size_t bbt_idx;

	if ((!dev) || (!bbt) || (nvm_addr_check(bbt->addr, &dev->geo))) {
		NVM_DEBUG("FAILED: invalid input");
		errno = EINVAL;
		return -1;
	}

	/* Refresh bbt entry in managed memory */
	if (!nvm_bbt_get(dev, addr, ret)) {
		NVM_DEBUG("FAILED: nvm_bbt_get failed");
		return -1;
	}

	/* Update bbt entry in managed memory with given bbt */
	bbt_idx = _bbt_idx(dev, addr);
	for (uint64_t i = 0; i < bbt->nblks; ++i)
		dev->bbts[bbt_idx]->blks[i] = bbt->blks[i];

	_refresh_counters(dev, dev->bbts[bbt_idx]);

	if (dev->bbts_cached)
		return 0;

	/* Flush bbt entry in managed memory to device */
	return nvm_bbt_flush(dev, addr, ret);
}

int nvm_bbt_mark(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 uint16_t flags, struct nvm_ret *ret)
{
	if (!dev->bbts_cached)
		return nvm_cmd_sbbt(dev, addrs, naddrs, flags, ret);

	/* Update bbt entries in managed memory */
	for (int i = 0; i < naddrs; ++i) {
		struct nvm_addr addr = addrs[i];

		size_t bbt_idx = _bbt_idx(dev, addr);
		size_t blk_idx = _blk_idx(dev, addr);

		if (!nvm_bbt_get(dev, addr, ret)) {
			NVM_DEBUG("FAILED: nvm_bbt_get failed");
			return -1;
		}

		dev->bbts[bbt_idx]->blks[blk_idx] = flags;

		_refresh_counters(dev, dev->bbts[bbt_idx]);
	}

	return 0;
}

struct nvm_bbt *nvm_bbt_alloc_cp(const struct nvm_bbt *bbt)
{
	struct nvm_bbt *new;

	if (!bbt) {
		NVM_DEBUG("FAILED: given bbt is invalid");
		errno = EINVAL;
		return NULL;
	}

	new = malloc(sizeof(*new) + sizeof(*(new->blks)) * bbt->nblks);
	if (!new) {
		NVM_DEBUG("FAILED: malloc 'new'");
		errno = ENOMEM;
		return NULL;
	}

	memcpy(new, bbt, sizeof(*new) + sizeof(*(new->blks)) * bbt->nblks);

	return new;
}

void nvm_bbt_free(struct nvm_bbt *bbt)
{
	if (!bbt)
		return;

	free(bbt);
}

void nvm_bbt_pr(const struct nvm_bbt *bbt)
{
	uint64_t npl_blks, nplanes;

	if (!bbt) {
		printf("bbt: ~\n");
		return;
	}

	nplanes = bbt->dev->geo.nplanes;
	npl_blks = bbt->nblks / nplanes;

	printf("bbt:\n");
	printf("  addr: "); nvm_addr_pr(bbt->addr);
	printf("  nblks: %"PRIu64"\n", bbt->nblks);
	printf("  npl_blks: %"PRIu64"\n", npl_blks);
	printf("  pl_blks:\n");
	for (uint64_t blk = 0; blk < bbt->nblks; ++blk) {
		if (!(blk % nplanes))
			printf("    %04"PRIu64": [ ", blk / nplanes);

		if (blk % nplanes)
			printf(", ");

		nvm_bbt_state_pr(bbt->blks[blk]);

		if (!((blk+1) % nplanes))
			printf(" ]\n");
	}
	printf("  nbad: %u\n", bbt->nbad);
	printf("  gbad: %u\n", bbt->ngbad);
	printf("  ndmrk: %u\n", bbt->ndmrk);
	printf("  nhmrk: %u\n", bbt->nhmrk);
}

void nvm_bbt_state_pr(int state)
{
	switch(state) {
	case NVM_BBT_FREE:
		printf("FREE(%d)", state);
		break;
	case NVM_BBT_BAD:
		printf(" BAD(%d)", state);
		break;
	case NVM_BBT_GBAD:
		printf("GBAD(%d)", state);
		break;
	case NVM_BBT_DMRK:
		printf("DMRK(%d)", state);
		break;
	case NVM_BBT_HMRK:
		printf("HMRK(%d)", state);
		break;
	default:
		printf("EINVAL(%d)", state);
		break;
	}
}

