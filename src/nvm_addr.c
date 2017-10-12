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

void nvm_ret_pr(const struct nvm_ret *ret)
{
	printf("nvm_ret: {");
	printf("result: 0x%x, ", ret->result);
	printf("status: %"PRIu64"",ret->status);
	printf("}\n");
}

void nvm_addr_pr(struct nvm_addr addr)
{
	printf("addr: {");
	printf("ppa: 0x%016"PRIx64", ", addr.ppa);
	printf("ch: %02d, ", addr.g.ch);
	printf("lun: %02d, ", addr.g.lun);
	printf("pl: %d, ", addr.g.pl);
	printf("blk: %04d, ", addr.g.blk);
	printf("pg: %03d, ", addr.g.pg);
	printf("sec: %d", addr.g.sec);
	printf("}\n");
}

void nvm_addr_prn(struct nvm_addr *addr, unsigned int naddrs)
{
	printf("naddrs: %d\n", naddrs);
	printf("addrs:\n");
	for (unsigned int i = 0; (i < naddrs) && addr; ++i) {
		printf("  - ");
		nvm_addr_pr(addr[i]);
	}
}

int nvm_addr_check(struct nvm_addr addr, const struct nvm_geo *geo)
{
	int exceeded = 0;

	if (addr.g.ch >= geo->nchannels) {
		exceeded |= NVM_BOUNDS_CHANNEL;
	}
	if (addr.g.lun >= geo->nluns) {
		exceeded |= NVM_BOUNDS_LUN;
	}
	if (addr.g.pl >= geo->nplanes) {
		exceeded |= NVM_BOUNDS_PLANE;
	}
	if (addr.g.blk >= geo->nblocks) {
		exceeded |= NVM_BOUNDS_BLOCK;
	}
	if (addr.g.pg >= geo->npages) {
		exceeded |= NVM_BOUNDS_PAGE;
	}
	if (addr.g.sec >= geo->nsectors) {
		exceeded |= NVM_BOUNDS_SECTOR;
	}

	return exceeded;
}

inline uint64_t nvm_addr_gen2dev(struct nvm_dev *dev, struct nvm_addr addr)
{
	uint64_t d_addr = 0;

	d_addr |= ((uint64_t)addr.g.ch) << dev->ppaf.n.ch_off;
	d_addr |= ((uint64_t)addr.g.lun) << dev->ppaf.n.lun_off;
	d_addr |= ((uint64_t)addr.g.pl) << dev->ppaf.n.pl_off;
	d_addr |= ((uint64_t)addr.g.blk) << dev->ppaf.n.blk_off;
	d_addr |= ((uint64_t)addr.g.pg) << dev->ppaf.n.pg_off;
	d_addr |= ((uint64_t)addr.g.sec) << dev->ppaf.n.sec_off;

	return d_addr;
}

uint64_t nvm_addr_gen2off(struct nvm_dev *dev, struct nvm_addr addr)
{
	return nvm_addr_gen2dev(dev, addr) << dev->ssw;
}

uint64_t nvm_addr_gen2lba(struct nvm_dev *dev, struct nvm_addr addr)
{
	return nvm_addr_gen2off(dev, addr) >> NVM_UNIVERSAL_SECT_SH;
}

uint64_t nvm_addr_dev2off(struct nvm_dev *dev, uint64_t addr)
{
	return addr << dev->ssw;
}

uint64_t nvm_addr_dev2lba(struct nvm_dev *dev, uint64_t addr)
{
	return nvm_addr_dev2off(dev, addr) >> NVM_UNIVERSAL_SECT_SH;
}

inline struct nvm_addr nvm_addr_dev2gen(struct nvm_dev *dev, uint64_t addr)
{
	struct nvm_addr gen;

	gen.ppa = 0;
	gen.g.ch = (addr & dev->mask.n.ch) >> dev->ppaf.n.ch_off;
	gen.g.lun |= (addr & dev->mask.n.lun) >> dev->ppaf.n.lun_off;
	gen.g.pl|= (addr & dev->mask.n.pl) >> dev->ppaf.n.pl_off;
	gen.g.blk |= (addr & dev->mask.n.blk) >> dev->ppaf.n.blk_off;
	gen.g.pg |= (addr & dev->mask.n.pg) >> dev->ppaf.n.pg_off;
	gen.g.sec |= (addr & dev->mask.n.sec) >> dev->ppaf.n.sec_off;

	return gen;
}

struct nvm_addr nvm_addr_off2gen(struct nvm_dev *dev, size_t off)
{
	return nvm_addr_dev2gen(dev, off >> dev->ssw);
}

struct nvm_addr nvm_addr_lba2gen(struct nvm_dev *dev, uint64_t off)
{
	return nvm_addr_off2gen(dev, off << NVM_UNIVERSAL_SECT_SH);
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

