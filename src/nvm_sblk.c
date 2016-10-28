/*
 * sblock - Spanning block functions
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
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
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

struct nvm_sblock* nvm_sblock_new(struct nvm_dev *dev,
				  int ch_bgn, int ch_end,
				  int lun_bgn, int lun_end,
				  int blk)
{
	struct nvm_sblock *sblk;
	struct nvm_geo geo;

	// TODO: verify bounds

	sblk = malloc(sizeof(*sblk));
	if (!sblk) {
		return NULL;
	}

	geo = nvm_dev_attr_geo(dev);
	
	sblk->dev = dev;

	sblk->bgn.g.ch = ch_bgn;
	sblk->bgn.g.lun = lun_bgn;
	sblk->bgn.g.blk = blk;
	sblk->bgn.g.pl = 0;
	sblk->bgn.g.pg = 0;
	sblk->bgn.g.sec = 0;

	sblk->end.g.ch = ch_end;
	sblk->end.g.lun = lun_end;
	sblk->end.g.blk = blk;
	sblk->end.g.pl = geo.nplanes - 1;
	sblk->end.g.pg = geo.npages - 1;
	sblk->end.g.sec = geo.nsectors - 1;

	if (ch_bgn != ch_end)
		sblk->span |= NVM_SBLOCK_SPAN_CH;

	if (lun_bgn != lun_end)
		sblk->span |= NVM_SBLOCK_SPAN_LUN;

	return sblk;
}

void nvm_sblock_free(struct nvm_sblock *sblk)
{
	free(sblk);
}

ssize_t nvm_sblock_erase(struct nvm_sblock *sblk)
{
	int ch;
	ssize_t nerr = 0;

	const int nplanes = nvm_sblock_attr_nplanes(sblk);
	const int nsectors = nvm_sblock_attr_nsectors(sblk);
	const int len = nplanes;

	for (ch = sblk->bgn.g.ch; ch <= sblk->end.g.ch; ++ch) {
		int lun;

		NVM_ADDR addr;
		
		addr.ppa = sblk->bgn.ppa;
		addr.g.ch = ch;

		for (lun = sblk->bgn.g.lun; lun <= sblk->end.g.lun; ++lun) {
			int pg;

			addr.g.lun = lun;

			for (pg = sblk->bgn.g.pg; pg <= sblk->end.g.pg; ++pg) {
				struct nvm_addr list[len];
				ssize_t err;
				int i;

				addr.g.pg = pg;

				for (i = 0; i < len; ++i) {
					list[i].ppa = addr.ppa;
					list[i].g.pl = i;
				}

				err = nvm_addr_erase(sblk->dev, list, len,
						     NVM_MAGIC_FLAG_DEFAULT);
				if (err) {
					NVM_DEBUG("sblk_erase err(%d)\n", err);
					++nerr;
				}
			}
		}
	}

	return -nerr;
}

ssize_t nvm_sblock_write(struct nvm_sblock *sblk, const void *buf, size_t pg,
			 size_t count)
{
	return 0;
}

ssize_t nvm_sblock_read(struct nvm_sblock *sblk, void *buf, size_t pg,
			size_t count)
{
	return 0;
}

void nvm_sblock_pr(struct nvm_sblock *sblk)
{
	printf("sblk {}\n");
}

