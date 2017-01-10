/*
 * addr - Sector addressing functions for write, read, and meta-data print
 *        Block addressing functions for erase and mark
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

void nvm_ret_pr(struct nvm_ret *ret)
{
	printf("nvm_ret { result(0x%x), status(%lu) }\n", ret->result,
	       ret->status);
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

	d_addr |= ((uint64_t)addr.g.ch) << dev->fmt.n.ch_ofz;
	d_addr |= ((uint64_t)addr.g.lun) << dev->fmt.n.lun_ofz;
	d_addr |= ((uint64_t)addr.g.pl) << dev->fmt.n.pl_ofz;
	d_addr |= ((uint64_t)addr.g.blk) << dev->fmt.n.blk_ofz;
	d_addr |= ((uint64_t)addr.g.pg) << dev->fmt.n.pg_ofz;
	d_addr |= ((uint64_t)addr.g.sec) << dev->fmt.n.sec_ofz;

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

inline struct nvm_addr nvm_addr_dev2gen(struct nvm_dev *dev, uint64_t addr)
{
	struct nvm_addr gen;

	gen.ppa = 0;
	gen.g.ch = (addr & dev->mask.n.ch) >> dev->fmt.n.ch_ofz;
	gen.g.lun |= (addr & dev->mask.n.lun) >> dev->fmt.n.lun_ofz;
	gen.g.pl|= (addr & dev->mask.n.pl) >> dev->fmt.n.pl_ofz;
	gen.g.blk |= (addr & dev->mask.n.blk) >> dev->fmt.n.blk_ofz;
	gen.g.pg |= (addr & dev->mask.n.pg) >> dev->fmt.n.pg_ofz;
	gen.g.sec |= (addr & dev->mask.n.sec) >> dev->fmt.n.sec_ofz;

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

static ssize_t nvm_addr_cmd(struct nvm_dev *dev, struct nvm_addr addrs[],
			    int naddrs, void *data, void *meta, uint16_t flags,
			    uint16_t opcode, struct nvm_ret *ret)
{
	struct nvm_user_vio ctl;
	uint64_t dev_addrs[naddrs];
	int i, err;

	if (naddrs > NVM_NADDR_MAX) {
		errno = EINVAL;
		return -1;
	}

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = opcode;
	ctl.control = flags | NVM_FLAG_DEFAULT;

	for (i = 0; i < naddrs; ++i) {	// Setup PPAs: Convert address format
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}
	ctl.nppas = naddrs - 1;		// Unnatural numbers: counting from zero
	ctl.ppa_list = naddrs == 1 ? dev_addrs[0] : (uint64_t)dev_addrs;

	ctl.addr = (uint64_t)data;	// Setup data
	ctl.data_len = data ? dev->geo.sector_nbytes * naddrs : 0;

	ctl.metadata = (uint64_t)meta;	// Setup metadata
	ctl.metadata_len = meta ? dev->geo.meta_nbytes * naddrs : 0;

	err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, &ctl);
#ifdef NVM_DEBUG_ENABLED
        nvm_addr_prn(addrs, naddrs);
	if (err || ctl.result || ctl.status) {
		printf("err(%d), ctl.result(%u), ctl.status(%llu)\n",
		       err, ctl.result, ctl.status);
	}
#endif
	if (ret) {
		ret->result = ctl.result;
		ret->status = ctl.status;
	}
	if (err) {	// Give up on IOCTL errors
		errno = EIO;
		return -1;
	}

	switch (ctl.result) {
	case 0x0:	// All good
		return 0;
	case 0x4700:	// As good as it gets..
		return 0;

	default:	// Everything else is an error
		errno = EIO;
		return -1;
	}
}

ssize_t nvm_addr_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       uint16_t flags, struct nvm_ret *ret)
{
	return nvm_addr_cmd(dev, addrs, naddrs, NULL, NULL, flags,
			    S12_OPC_ERASE, ret);
}

ssize_t nvm_addr_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       const void *data, const void *meta, uint16_t flags,
		       struct nvm_ret *ret)
{
	char *cdata = (char *)data;
        char *cmeta = (char *)meta;

	return nvm_addr_cmd(dev, addrs, naddrs, cdata, cmeta, flags,
			    S12_OPC_WRITE, ret);
}

ssize_t nvm_addr_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		      void *data, void *meta, uint16_t flags,
		      struct nvm_ret *ret)
{
	return nvm_addr_cmd(dev, addrs, naddrs, data, meta, flags,
			    S12_OPC_READ, ret);
}

void nvm_addr_fmt_pr(struct nvm_addr_fmt *fmt)
{
	printf("fmt {\n");
	printf("  ch_ofz(%02d),  ch_len(%02d), \n",
	       fmt->n.ch_ofz, fmt->n.ch_len);
	printf(" lun_ofz(%02d), lun_len(%02d), \n",
	       fmt->n.lun_ofz, fmt->n.lun_len);
	printf("  pl_ofz(%02d),  pl_len(%02d), \n",
	       fmt->n.pl_ofz, fmt->n.pl_len);
	printf(" blk_ofz(%02d), blk_len(%02d), \n",
	       fmt->n.blk_ofz, fmt->n.blk_len);
	printf("  pg_ofz(%02d),  pg_len(%02d), \n",
	       fmt->n.pg_ofz, fmt->n.pg_len);
	printf(" sec_ofz(%02d), sec_len(%02d), \n",
	       fmt->n.sec_ofz, fmt->n.sec_len);
	printf("}\n");
}


void nvm_addr_fmt_mask_pr(struct nvm_addr_fmt_mask *mask)
{
	printf("fmt-mask {\n");
	printf("  ch("NVM_I64_FMT"),\n", NVM_I64_TO_STR(mask->n.ch));
	printf(" lun("NVM_I64_FMT"),\n", NVM_I64_TO_STR(mask->n.lun));
	printf("  pl("NVM_I64_FMT"),\n", NVM_I64_TO_STR(mask->n.pl));
	printf(" blk("NVM_I64_FMT"),\n", NVM_I64_TO_STR(mask->n.blk));
	printf("  pg("NVM_I64_FMT"),\n", NVM_I64_TO_STR(mask->n.pg));
	printf(" sec("NVM_I64_FMT")\n", NVM_I64_TO_STR(mask->n.sec));
	printf("}\n");
}

void nvm_addr_pr(struct nvm_addr addr)
{
	printf("(0x%016lx){ ch(%02d), lun(%02d), pl(%d), ",
	       addr.ppa, addr.g.ch, addr.g.lun, addr.g.pl);
	printf("blk(%04d), pg(%03d), sec(%d) }\n",
	       addr.g.blk, addr.g.pg, addr.g.sec);
}

void nvm_addr_prn(struct nvm_addr *addr, unsigned int naddrs)
{
	printf("naddrs(%d) {\n", naddrs);
	for (int i = 0; i < naddrs; ++i) {
		printf(" %02d: ", i); nvm_addr_pr(addr[i]);
	}
	printf("}\n");
}

