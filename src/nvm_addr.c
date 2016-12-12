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

void nvm_ret_pr(NVM_RET *ret)
{
	printf("nvm_ret { result(0x%x), status(%lu) }\n", ret->result,
	       ret->status);
}

int nvm_addr_check(NVM_ADDR addr, NVM_GEO geo)
{
	int exceeded = 0;

	if (addr.g.ch >= geo.nchannels) {
		exceeded |= NVM_BOUNDS_CHANNEL;
	}
	if (addr.g.lun >= geo.nluns) {
		exceeded |= NVM_BOUNDS_LUN;
	}
	if (addr.g.pl >= geo.nplanes) {
		exceeded |= NVM_BOUNDS_PLANE;
	}
	if (addr.g.blk >= geo.nblocks) {
		exceeded |= NVM_BOUNDS_BLOCK;
	}
	if (addr.g.pg >= geo.npages) {
		exceeded |= NVM_BOUNDS_PAGE;
	}
	if (addr.g.sec >= geo.nsectors) {
		exceeded |= NVM_BOUNDS_SECTOR;
	}

	return exceeded;
}

inline struct nvm_addr nvm_addr_gen2dev(struct nvm_dev *dev,
					       struct nvm_addr addr)
{
	struct nvm_addr d_addr;

	d_addr.ppa = 0;
	d_addr.ppa |= ((uint64_t)addr.g.ch) << dev->fmt.n.ch_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.lun) << dev->fmt.n.lun_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.pl) << dev->fmt.n.pl_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.blk) << dev->fmt.n.blk_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.pg) << dev->fmt.n.pg_ofz;
	d_addr.ppa |= ((uint64_t)addr.g.sec) << dev->fmt.n.sec_ofz;

	return d_addr;
}

size_t nvm_addr_gen2lba(struct nvm_dev *dev, NVM_ADDR addr)
{
	size_t lba = 0;

	// [ch][lun][blk][pg][pl][sec]~nbytes = offset

	lba += dev->lba_map.channel_nbytes * addr.g.ch;
	lba += dev->lba_map.lun_nbytes * addr.g.lun;
	lba += dev->lba_map.plane_nbytes * addr.g.pl;
	lba += dev->lba_map.block_nbytes * addr.g.blk;
	lba += dev->lba_map.page_nbytes * addr.g.pg;
	lba += dev->lba_map.sector_nbytes * addr.g.sec;

	return lba;
}

NVM_ADDR nvm_addr_lba2gen(struct nvm_dev *dev, size_t lba)
{
	NVM_ADDR addr;
	struct nvm_lba_map *map = &dev->lba_map;

	addr.ppa = 0;

	addr.g.ch = lba / map->channel_nbytes;
	addr.g.lun = (lba % map->channel_nbytes) / map->lun_nbytes;
	addr.g.blk = (lba % map->lun_nbytes) / map->block_nbytes;
	addr.g.pg = (lba % map->block_nbytes) / map->page_nbytes;
	addr.g.pl = (lba % map->page_nbytes) / map->plane_nbytes;
	addr.g.sec = (lba % map->plane_nbytes) / map->sector_nbytes;

	return addr;
}

static ssize_t nvm_addr_cmd(struct nvm_dev *dev, struct nvm_addr addrs[],
			    int len, void *data, void *meta, uint16_t flags,
			    uint16_t opcode, struct nvm_return *ret)
{
	struct nvm_user_vio ctl;
	struct nvm_addr dev_addrs[len];
	int i, err;

	memset(&ctl, 0, sizeof(ctl));
	ctl.opcode = opcode;
	ctl.control = flags | NVM_MAGIC_FLAG_DEFAULT;

	for (i = 0; i < len; ++i) {	// Setup PPAs: Convert address format
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}
	ctl.nppas = len - 1;		// Unnatural numbers: counting from zero
	ctl.ppa_list = len == 1 ? dev_addrs[0].ppa : (uint64_t)dev_addrs;

	ctl.addr = (uint64_t)data;	// Setup data
	ctl.data_len = data ? dev->geo.nbytes * len : 0;

	ctl.metadata = (uint64_t)meta;	// Setup metadata
	ctl.metadata_len = meta ? dev->geo.meta_nbytes * len : 0;

	err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, &ctl);
#ifdef NVM_DEBUG_ENABLED
	if (err || ctl.result || ctl.status) {
		NVM_DEBUG("err(%d), ctl: result(0x%x), status(%llu), nppas(%d)",
			  err, ctl.result, ctl.status, ctl.nppas);
		for (i = 0; i < len; ++i)
			nvm_addr_pr(addrs[i]);
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

ssize_t nvm_addr_erase(NVM_DEV dev, NVM_ADDR addrs[], int naddrs, uint16_t flags,
		       NVM_RET *ret)
{
	return nvm_addr_cmd(dev, addrs, naddrs, NULL, NULL, flags,
			    S12_OPC_ERASE, ret);
}

ssize_t nvm_addr_write(struct nvm_dev *dev, NVM_ADDR addrs[], int naddrs,
		       const void *data, const void *meta, uint16_t flags,
		       NVM_RET *ret)
{
	char *cdata = (char *)data;
        char *cmeta = (char *)meta;

	return nvm_addr_cmd(dev, addrs, naddrs, cdata, cmeta, flags,
			    S12_OPC_WRITE, ret);
}

ssize_t nvm_addr_read(struct nvm_dev *dev, NVM_ADDR addrs[], int naddrs,
		      void *data, void *meta, uint16_t flags, NVM_RET *ret)
{
	return nvm_addr_cmd(dev, addrs, naddrs, data, meta, flags,
			    S12_OPC_READ, ret);
}

ssize_t nvm_addr_mark(NVM_DEV dev, NVM_ADDR addrs[], int naddrs, uint16_t flags,
		      NVM_RET *ret)
{
	switch (flags) {
	case 0x0:
	case 0x1:
	case 0x2:
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return nvm_bbt_mark(dev, addrs, naddrs, flags, ret);
}

void nvm_addr_fmt_pr(struct nvm_addr_fmt *fmt)
{
	printf("fmt {\n");
	printf(" ch_ofz(%d), ch_len(%d)", fmt->n.ch_ofz, fmt->n.ch_len);
	printf(",\n lun_ofz(%d), lun_len(%d)", fmt->n.lun_ofz, fmt->n.lun_len);
	printf(",\n pl_ofz(%d), pl_len(%d)", fmt->n.pl_ofz, fmt->n.pl_len);
	printf(",\n blk_ofz(%d), blk_len(%d)", fmt->n.blk_ofz, fmt->n.blk_len);
	printf(",\n pg_ofz(%d), pg_len(%d)", fmt->n.pg_ofz, fmt->n.pg_len);
	printf(",\n sec_ofz(%d), sec_len(%d)", fmt->n.sec_ofz, fmt->n.sec_len);
	printf("\n}\n");
}

void nvm_addr_pr(struct nvm_addr addr)
{
	printf("(0x%016lx){ ch(%02d), lun(%02d), pl(%d), ",
	       addr.ppa, addr.g.ch, addr.g.lun, addr.g.pl);
	printf("blk(%04d), pg(%03d), sec(%d) }\n",
	       addr.g.blk, addr.g.pg, addr.g.sec);
}
