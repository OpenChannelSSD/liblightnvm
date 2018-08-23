/*
 * addr - Sector addressing functions for write, read, and meta-data print
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
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_debug.h>
#include <nvm_utils.h>

void nvm_addr_pr(const struct nvm_addr addr)
{
	printf("0x%016"PRIx64, addr.val);
}

static void nvm_addr_pr_s12(struct nvm_addr addr)
{
	printf("{");
	printf("val: 0x%016"PRIx64", ", addr.ppa);
	printf("ch: %02d, ", addr.g.ch);
	printf("lun: %02d, ", addr.g.lun);
	printf("blk: %04d, ", addr.g.blk);
	printf("pl: %d, ", addr.g.pl);
	printf("pg: %03d, ", addr.g.pg);
	printf("sec: %d", addr.g.sec);
	printf("}\n");
}

static void nvm_addr_pr_s20(struct nvm_addr addr)
{
	printf("{");
	printf("val: 0x%016"PRIx64", ", addr.ppa);
	printf("pugrp: %02d, ", addr.l.pugrp);
	printf("punit: %02d, ", addr.l.punit);
	printf("chunk: %04d, ", addr.l.chunk);
	printf("sectr: %04d", addr.l.sectr);
	printf("}\n");
}

void nvm_addr_prn(const struct nvm_addr *addr, unsigned int naddrs,
		  const struct nvm_dev *dev)
{
	printf("naddrs: %d\n", naddrs);
	printf("addrs:\n");
	for (unsigned int i = 0; (i < naddrs) && addr; ++i) {
		printf("  - ");
		switch(nvm_dev_get_verid(dev)) {
			case NVM_SPEC_VERID_12:
				nvm_addr_pr_s12(addr[i]);
				break;

			case NVM_SPEC_VERID_20:
				nvm_addr_pr_s20(addr[i]);
				break;

			default:
				printf("UNSUPPORTED_VERID\n");
		}
	}
}

int nvm_addr_check(struct nvm_addr addr, const struct nvm_dev *dev)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	int exceeded = 0;

	switch(dev->verid) {
	case NVM_SPEC_VERID_12:
		if (addr.g.ch >= geo->nchannels)
			exceeded |= NVM_BOUNDS_CHANNEL;
		if (addr.g.lun >= geo->nluns)
			exceeded |= NVM_BOUNDS_LUN;
		if (addr.g.pl >= geo->nplanes)
			exceeded |= NVM_BOUNDS_PLANE;
		if (addr.g.blk >= geo->nblocks)
			exceeded |= NVM_BOUNDS_BLOCK;
		if (addr.g.pg >= geo->npages)
			exceeded |= NVM_BOUNDS_PAGE;
		if (addr.g.sec >= geo->nsectors)
			exceeded |= NVM_BOUNDS_SECTOR;
		return exceeded;

	case NVM_SPEC_VERID_20:
		if (addr.l.pugrp >= geo->l.npugrp)
			exceeded |= NVM_BOUNDS_CHANNEL;
		if (addr.l.punit >= geo->l.npunit)
			exceeded |= NVM_BOUNDS_LUN;
		if (addr.l.chunk >= geo->l.nchunk)
			exceeded |= NVM_BOUNDS_PLANE;
		if (addr.l.sectr >= geo->l.nsectr)
			exceeded |= NVM_BOUNDS_BLOCK;
		return exceeded;
	}

	NVM_DEBUG("FAILED: unsupported verid: %d", dev->verid);
	errno = EINVAL;
	return -1;
}

inline uint64_t nvm_addr_gen2dev(struct nvm_dev *dev, struct nvm_addr addr)
{
	if (dev->verid == NVM_SPEC_VERID_20) {
		uint64_t d_addr = 0;

		d_addr |= ((uint64_t)addr.l.pugrp) << dev->lbaz.pugrp;
		d_addr |= ((uint64_t)addr.l.punit) << dev->lbaz.punit;
		d_addr |= ((uint64_t)addr.l.chunk) << dev->lbaz.chunk;
		d_addr |= ((uint64_t)addr.l.sectr) << dev->lbaz.sectr;

		return d_addr;
	} else {

		uint64_t d_addr = 0;

		d_addr |= ((uint64_t)addr.g.ch) << dev->ppaf.n.ch_off;
		d_addr |= ((uint64_t)addr.g.lun) << dev->ppaf.n.lun_off;
		d_addr |= ((uint64_t)addr.g.pl) << dev->ppaf.n.pl_off;
		d_addr |= ((uint64_t)addr.g.blk) << dev->ppaf.n.blk_off;
		d_addr |= ((uint64_t)addr.g.pg) << dev->ppaf.n.pg_off;
		d_addr |= ((uint64_t)addr.g.sec) << dev->ppaf.n.sec_off;

		return d_addr;
	}
}

uint64_t nvm_addr_gen2off(struct nvm_dev *dev, struct nvm_addr addr)
{
	return nvm_addr_gen2dev(dev, addr) << dev->ssw;
}

uint64_t nvm_addr_dev2off(struct nvm_dev *dev, uint64_t addr)
{
	return addr << dev->ssw;
}

inline struct nvm_addr nvm_addr_dev2gen(struct nvm_dev *dev, uint64_t addr)
{
	if (dev->verid == NVM_SPEC_VERID_20) {// TODO: Fix this for spec. 2.0
		struct nvm_addr gen = { .val = 0 };

		gen.l.pugrp = (addr & dev->lbam.pugrp) >> dev->lbaz.pugrp;
		gen.l.punit = (addr & dev->lbam.punit) >> dev->lbaz.punit;
		gen.l.chunk = (addr & dev->lbam.chunk) >> dev->lbaz.chunk;
		gen.l.sectr = (addr & dev->lbam.sectr) >> dev->lbaz.sectr;

		return gen;
	} else {
		struct nvm_addr gen = { .val = 0 };

		gen.g.ch = (addr & dev->mask.n.ch) >> dev->ppaf.n.ch_off;
		gen.g.lun = (addr & dev->mask.n.lun) >> dev->ppaf.n.lun_off;
		gen.g.pl= (addr & dev->mask.n.pl) >> dev->ppaf.n.pl_off;
		gen.g.blk = (addr & dev->mask.n.blk) >> dev->ppaf.n.blk_off;
		gen.g.pg = (addr & dev->mask.n.pg) >> dev->ppaf.n.pg_off;
		gen.g.sec = (addr & dev->mask.n.sec) >> dev->ppaf.n.sec_off;

		return gen;
	}
}

uint64_t nvm_addr_gen2lpo(struct nvm_dev *dev, struct nvm_addr addr)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);

	uint64_t idx = 0;

	idx += addr.l.pugrp * geo->l.npunit * geo->l.nchunk;
	idx += addr.l.punit * geo->l.nchunk;
	idx += addr.l.chunk;

	return idx * sizeof(struct nvm_spec_rprt_descr);
}

struct nvm_addr nvm_addr_lpo2gen(struct nvm_dev *dev, uint64_t lpo)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);

	const size_t i = lpo / sizeof(struct nvm_spec_rprt_descr);

	const struct nvm_addr addr = {
	.l.sectr = 0,
	.l.chunk = i % geo->l.nchunk,
	.l.punit = (i / geo->l.nchunk) % geo->l.npunit,
	.l.pugrp = ((i / geo->l.nchunk) / geo->l.npunit) % geo->l.npugrp
	};

	return addr;
}

struct nvm_addr nvm_addr_off2gen(struct nvm_dev *dev, size_t off)
{
	return nvm_addr_dev2gen(dev, off >> dev->ssw);
}

ssize_t nvm_addr_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       uint16_t flags, struct nvm_ret *ret)
{
	return nvm_cmd_erase(dev, addrs, naddrs, flags, ret);
}

ssize_t nvm_addr_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       const void *data, const void *meta, uint16_t flags,
		       struct nvm_ret *ret)
{
	char *cdata = (char *)data;
	char *cmeta = (char *)meta;

	return nvm_cmd_write(dev, addrs, naddrs, cdata, cmeta, flags, ret);
}

ssize_t nvm_addr_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		      void *data, void *meta, uint16_t flags,
		      struct nvm_ret *ret)
{
	return nvm_cmd_read(dev, addrs, naddrs, data, meta, flags, ret);
}
