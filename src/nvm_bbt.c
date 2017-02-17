/*
 * bbt - Bad-block-table functions
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
 * Copyright (C) 2017 Simon A. F. Lund <slund@cnexlabs.com>
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
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

/**
 * Hack'n'slashed in here...
 */
struct krnl_bbt {
	uint8_t		tblid[4];
	uint16_t	verid;
	uint16_t	revid;
	uint32_t	rvsd1;
	uint32_t	tblks;
	uint32_t	tfact;
	uint32_t	tgrown;
	uint32_t	tdresv;
	uint32_t	thresv;
	uint32_t	rsvd2[8];
	uint8_t		blk[0];
};

void krnl_bbt_pr(struct krnl_bbt *bbt)
{
	if (!bbt) {
		printf("krnl_bbt(NULL)\n");
		return;
	}

	printf("tblkid {%c, %c, %c, %c}\n",
	       bbt->tblid[0], bbt->tblid[1], bbt->tblid[2], bbt->tblid[3]);
	printf("verid(%u)\n", bbt->verid);
	printf("revid(%u)\n", bbt->revid);
	printf("rvsd1(%u)\n", bbt->rvsd1);
	printf("tblks(%u)\n", bbt->tblks);
	printf("tfact(%u)\n", bbt->tfact);
	printf("tgrown(%u)\n", bbt->tgrown);

	printf("tdresv(%u)\n", bbt->tdresv);
	printf("thresv(%u)\n", bbt->thresv);
	printf("rsvd2(..)\n");

	printf("blk[] (notgood) {\n");
	for (int i = 0; i < bbt->tblks; ++i) {
		if (i)
			printf("\n");
		printf("i(%d) = %u", i, bbt->blk[i]);
	}
	printf("}\n");
}

/**
 * Retrieve bad-block-table from kernel and update the blk-list of the given
 * bbt
 *
 * @returns 0 on success. -1 on error and errno set to indicate the error
 */
static inline int krnl_bbt_get(struct nvm_bbt *bbt, struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {};
	struct krnl_bbt *k_bbt;
	size_t krnl_bbt_sz;
	int err;

	krnl_bbt_sz = sizeof(*k_bbt) + sizeof(*(k_bbt->blk)) * bbt->nblks;
	k_bbt = nvm_buf_alloc(&bbt->dev->geo, krnl_bbt_sz);
	if (!k_bbt) {
		NVM_DEBUG("FAILED: malloc k_bbt failed");
		errno = ENOMEM;
		return -1;
	}

	cmd.vadmin.opcode = S12_OPC_GET_BBT;
	cmd.vadmin.addr = (uint64_t)k_bbt;
	cmd.vadmin.data_len = krnl_bbt_sz;
	cmd.vadmin.ppa_list = nvm_addr_gen2dev(bbt->dev, bbt->addr);
	cmd.vadmin.nppas = 0;

	err = bbt->dev->be->vadmin(bbt->dev, &cmd, ret);
	if (err || (k_bbt->tblks != bbt->nblks)) {
		NVM_DEBUG("FAILED: be execution failed");
		errno = EIO;
		free(k_bbt);
		return -1;
	}
	if (!(k_bbt->tblid[0] == 'B' && k_bbt->tblid[1] == 'B' &&
	      k_bbt->tblid[2] == 'L' && k_bbt->tblid[3] == 'T')) {
		NVM_DEBUG("FAILED: invalid format of returned bbt");
		errno = EIO;
		free(k_bbt);
		return -1;
	}

	for (int i = 0; i < bbt->nblks; ++i)
		bbt->blks[i] = k_bbt->blk[i];

	free(k_bbt);

	return 0;
}

static inline int krnl_bbt_mark(struct nvm_dev *dev, struct nvm_addr addrs[],
				int naddrs, uint16_t flags, struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {};
	uint64_t dev_addrs[naddrs];
	int err;

	switch(flags) {
	case NVM_BBT_FREE:
	case NVM_BBT_BAD:
	case NVM_BBT_DMRK:
	case NVM_BBT_GBAD:
	case NVM_BBT_HMRK:
		break;
	default:
		NVM_DEBUG("FAILED: invalid mark");
		errno = EINVAL;
		return -1;
	}

	if (naddrs > NVM_NADDR_MAX) {
		NVM_DEBUG("FAILED: naddrs > NVM_NADDR_MAX");
		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < naddrs; ++i) {	// Setup PPAs: Convert format
		if (nvm_addr_check(addrs[i], &dev->geo)) {
			NVM_DEBUG("FAILED: invalid addrs[i]");
			errno = EINVAL;
			return -1;
		}
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}

	cmd.vadmin.opcode = S12_OPC_SET_BBT;	// Construct command
	cmd.vadmin.control = flags;
	cmd.vadmin.nppas = naddrs - 1;		// Unnatural numbers: counting from zero
	cmd.vadmin.ppa_list = naddrs == 1 ? dev_addrs[0] : (uint64_t)dev_addrs;

	err = dev->be->vadmin(dev, &cmd, ret);
	if (err) {
		NVM_DEBUG("FAILED: be execution failed");
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_bbt_flush(struct nvm_dev *dev, struct nvm_addr addr,
		  struct nvm_ret *ret)
{
	const struct nvm_bbt *bbt;
	struct nvm_bbt *krnl;
	size_t bbt_idx;
	int err;

	if ((!dev) || (nvm_addr_check(addr, &dev->geo))) {
		NVM_DEBUG("FAILED: !dev or nvm_addr_check failed");
		errno = EINVAL;
		return -1;
	}

	bbt_idx = _bbt_idx(dev, addr);

	bbt = dev->bbts[bbt_idx];
	if (!bbt)
		return 0;			// Nothing to flush

	krnl = nvm_bbt_alloc_cp(bbt);
	if (!krnl) {
		NVM_DEBUG("FAILED: nvm_bbt_alloc_cp failed");
		return -1;			// Propagate `errno`
	}

	err = krnl_bbt_get(krnl, ret);
	if (err) {
		NVM_DEBUG("FAILED: krnl_bbt_get failed");
		return -1;			// Propagate `errno`
	}

	if (bbt->nblks != krnl->nblks) {
		NVM_DEBUG("FAILED: bbt->blks != krnl->nblks");
		errno = EINVAL;
		return -1;
	}
	
	for (int i = 0; i < bbt->nblks; ++i) {	// Update on device
		struct nvm_addr blk_addr;

		if (bbt->blks[i] == krnl->blks[i])
			continue;		// Ignore same state

		// Convert "i -> (blk, pl)" and submit changed state
		blk_addr.ppa = bbt->addr.ppa;
		blk_addr.g.blk = i / dev->geo.nplanes;
		blk_addr.g.pl = i % dev->geo.nplanes;

		if (krnl_bbt_mark(dev, &blk_addr, 1, bbt->blks[i], ret)) {
			NVM_DEBUG("FAILED: krnl_bbt_mark failed");
			return -1;		// Propagate `errno`
		}
	}

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
	size_t bbt_idx;
	int err;

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
		struct nvm_bbt *bbt;

		bbt = malloc(sizeof(*bbt));
		if (!bbt) {
			NVM_DEBUG("FAILED: malloc of bbt failed");
			errno = ENOMEM;
			return NULL;
		}

		bbt->dev = dev;
		bbt->addr = addr;
		bbt->nblks = dev->geo.nblocks * dev->geo.nplanes;
		bbt->blks = malloc(sizeof(*bbt->blks) * bbt->nblks);
		if (!bbt->blks) {
			NVM_DEBUG("FAILED: malloc of bbt->blks");
			free(bbt);
			errno = ENOMEM;
			return NULL;
		}

		dev->bbts[bbt_idx] = bbt;
	}

	/* Update bbt entry in managed memory area with bbt from device */
	err = krnl_bbt_get(dev->bbts[bbt_idx], ret);
	if (err) {
		free(dev->bbts[bbt_idx]->blks);
		free(dev->bbts[bbt_idx]);
		dev->bbts[bbt_idx] = NULL;

		return NULL;
	}

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
	for (int i = 0; i < bbt->nblks; ++i)
		dev->bbts[bbt_idx]->blks[i] = bbt->blks[i];

	/* Flush bbt entry in managed memory to device */
	if (!dev->bbts_cached)
		return nvm_bbt_flush(dev, addr, ret);

	return 0;
}

int nvm_bbt_mark(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 uint16_t flags, struct nvm_ret *ret)
{
	if (!dev->bbts_cached)
		return krnl_bbt_mark(dev, addrs, naddrs, flags, ret);

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
	}

	return 0;
}

struct nvm_bbt *nvm_bbt_alloc_cp(const struct nvm_bbt *bbt)
{
	struct nvm_bbt *new;

	if ((!bbt) || (!bbt->blks)) {
		NVM_DEBUG("FAILED: given bbt is invalid");
		errno = EINVAL;
		return NULL;
	}

	new = malloc(sizeof(*new));
	if (!new) {
		NVM_DEBUG("FAILED: malloc 'new'");
		errno = ENOMEM;
		return NULL;
	}

	new->dev = bbt->dev;
	new->addr = bbt->addr;
	new->nblks = bbt->nblks;

	new->blks = malloc(sizeof(*(new->blks)) * bbt->nblks);
	if (!new->blks) {
		NVM_DEBUG("FAILED: malloc new->nblks");
		free(new);
		errno = ENOMEM;
		return NULL;
	}

	for (uint64_t i = 0; i < bbt->nblks; ++i)
		new->blks[i] = bbt->blks[i];

	return new;
}

void nvm_bbt_free(struct nvm_bbt *bbt)
{
	if (!bbt)
		return;

	free(bbt->blks);
	free(bbt);
}

void nvm_bbt_pr(const struct nvm_bbt *bbt)
{
	int nnotfree = 0;

	if (!bbt) {
		printf("bbt { NULL }\n");
		return;
	}

	printf("bbt {\n");
	printf("  addr"); nvm_addr_pr(bbt->addr);
	printf("  nblks(%lu) {", bbt->nblks);
	for (int i = 0; i < bbt->nblks; i += bbt->dev->geo.nplanes) {
		int blk = i / bbt->dev->geo.nplanes;

		printf("\n    blk(%04d): [ ", blk);
		for (int blk = i; blk < (i+ bbt->dev->geo.nplanes); ++blk) {
			nvm_bbt_state_pr(bbt->blks[blk]);
			printf(" ");
			if (bbt->blks[blk]) {
				++nnotfree;
			}
		}
		printf("]");
	}
	printf("\n  }\n");
	printf("  #notfree(%d)\n", nnotfree);
	printf("}\n");
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
		printf("DBAD(%d)", state);
		break;
	case NVM_BBT_HMRK:
		printf("HBAD(%d)", state);
		break;
	default:
		printf("EINVAL(%d)", state);
		break;
	}
}
