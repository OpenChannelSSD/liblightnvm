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
#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#endif

struct nvm_sblk* nvm_sblk_new(struct nvm_dev *dev,
                              int ch_bgn, int ch_end,
                              int lun_bgn, int lun_end,
                              int blk)
{
	struct nvm_sblk *sblk;
	struct nvm_geo dev_geo = nvm_dev_attr_geo(dev);

	if (ch_bgn < 0 || ch_bgn > ch_end || ch_end >= dev_geo.nchannels) {
		NVM_DEBUG("invalid channel span");
		return NULL;
	}
	if (lun_bgn < 0 || lun_bgn > lun_end || lun_end >= dev_geo.nluns) {
		NVM_DEBUG("invalid lun span");
		return NULL;
	}
	if (blk < 0 || blk >= dev_geo.nblocks) {
		NVM_DEBUG("invalid block");
		return NULL;
	}

	sblk = malloc(sizeof(*sblk));
	if (!sblk) {
		return NULL;
	}
	
	sblk->dev = dev;

	sblk->bgn.g.ch = ch_bgn;	/* Construct span */
	sblk->bgn.g.lun = lun_bgn;
	sblk->bgn.g.blk = blk;
	sblk->bgn.g.pl = 0;
	sblk->bgn.g.pg = 0;
	sblk->bgn.g.sec = 0;

	sblk->end.g.ch = ch_end;
	sblk->end.g.lun = lun_end;
	sblk->end.g.blk = blk;
	sblk->end.g.pl = dev_geo.nplanes - 1;
	sblk->end.g.pg = dev_geo.npages - 1;
	sblk->end.g.sec = dev_geo.nsectors - 1;

	sblk->geo = dev_geo;		/* Inherit geometry from device */

	/* Overwrite with channels and luns */
	sblk->geo.nchannels = (sblk->end.g.ch - sblk->bgn.g.ch) + 1;
	sblk->geo.nluns = (sblk->end.g.lun - sblk->bgn.g.lun) + 1;
	sblk->geo.nblocks = 1; // For each ch/lun there is only one block

	/* Derive total number of bytes in sblk */
	sblk->geo.tbytes = sblk->geo.nchannels * sblk->geo.nluns * \
			   sblk->geo.nplanes * sblk->geo.nblocks * \
			   sblk->geo.npages * sblk->geo.nsectors * \
			   sblk->geo.nbytes;

	return sblk;
}

void nvm_sblk_free(struct nvm_sblk *sblk)
{
	free(sblk);
}

ssize_t nvm_sblk_erase(struct nvm_sblk *sblk)
{
	const struct nvm_geo geo = nvm_sblk_attr_geo(sblk);

	const int nchannels = geo.nchannels;
	const int nluns = geo.nluns;
	const int nplanes = geo.nplanes;
	const int naddrs = nluns * nplanes;

	const struct nvm_addr bgn = sblk->bgn;
	const struct nvm_addr end = sblk->end;

	ssize_t nerr = 0;

	#pragma omp parallel for num_threads(nchannels) schedule(static) reduction(+:nerr)
	for (int ch = bgn.g.ch; ch <= end.g.ch; ++ch) {
		ssize_t err;
		struct nvm_addr addrs[naddrs];

		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = bgn.ppa;
			addrs[i].g.ch = ch;
			addrs[i].g.lun = (i / nplanes) % nluns + bgn.g.lun;
			addrs[i].g.pl = i % nplanes;
			// blk is fixed and inherited from bgn
			// pg is fixed and inherited from bgn (0)
			// sec is fixed and inherited from bgn (0)
		}
		
		err = nvm_addr_erase(sblk->dev, addrs, naddrs,
				     NVM_MAGIC_FLAG_DEFAULT);
		if (err) {
			NVM_DEBUG("sblk_erase err(%d)\n", err);
			++nerr;
		}
	}

	return -nerr;
}

ssize_t nvm_sblk_write(struct nvm_sblk *sblk, const void *buf, size_t count)
{
	const struct nvm_geo geo = nvm_sblk_attr_geo(sblk);

	const int nchannels = geo.nchannels;
	const int nluns = geo.nluns;
	const int nplanes = geo.nplanes;
	const int nsectors = geo.nsectors;
	const int nbytes = geo.nbytes;
	const int naddrs = geo.nluns * geo.nplanes * geo.nsectors;

	const struct nvm_addr bgn = sblk->bgn;
	const struct nvm_addr end = sblk->end;

	ssize_t nerr = 0;

	#pragma omp parallel for num_threads(nchannels) schedule(static) reduction(+:nerr)
	for (int ch = bgn.g.ch; ch <= end.g.ch; ++ch) {

		struct nvm_addr addrs[naddrs];
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = bgn.ppa;
			addrs[i].g.ch = ch;
			addrs[i].g.lun = ((i / nsectors) / nplanes) % nluns + bgn.g.lun;
			addrs[i].g.pl = (i / nsectors) % nplanes;
			// blk is fixed and inherited from bgn
			addrs[i].g.sec = i % nsectors;
		}
		
		for (int pg = 0; pg < count; ++pg) {
			ssize_t err;

			for (int i = 0; i < naddrs; ++i) {
				addrs[i].g.pg = pg;
			}

			err = nvm_addr_write(sblk->dev, addrs, naddrs,
					     buf + naddrs * nbytes * pg,
					     NVM_MAGIC_FLAG_DEFAULT);
			if (err) {
				NVM_DEBUG("sblk_write err(%d)\n", err);
				++nerr;
			}
		}
	}

	return -nerr;
}

ssize_t nvm_sblk_read(struct nvm_sblk *sblk, void *buf, size_t count)
{
	const struct nvm_geo geo = nvm_sblk_attr_geo(sblk);

	const int nchannels = geo.nchannels;
	const int nluns = geo.nluns;
	const int nplanes = geo.nplanes;
	const int nsectors = geo.nsectors;
	const int nbytes = geo.nbytes;
	const int naddrs = geo.nluns * geo.nplanes * geo.nsectors;

	const struct nvm_addr bgn = sblk->bgn;
	const struct nvm_addr end = sblk->end;

	ssize_t nerr = 0;

	#pragma omp parallel for num_threads(nchannels) schedule(static) reduction(+:nerr)
	for (int ch = bgn.g.ch; ch <= end.g.ch; ++ch) {

		struct nvm_addr addrs[naddrs];
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = bgn.ppa;
			addrs[i].g.ch = ch;
			addrs[i].g.lun = ((i / nsectors) / nplanes) % nluns + bgn.g.lun;
			addrs[i].g.pl = (i / nsectors) % nplanes;
			// blk is fixed and inherited from bgn
			addrs[i].g.sec = i % nsectors;
		}
		
		for (int pg = 0; pg < count; ++pg) {
			ssize_t err;

			for (int i = 0; i < naddrs; ++i) {
				addrs[i].g.pg = pg;
			}

			err = nvm_addr_read(sblk->dev, addrs, naddrs,
					    buf + naddrs * nbytes * pg,
					    NVM_MAGIC_FLAG_DEFAULT);
			if (err) {
				NVM_DEBUG("sblk_read err(%d)\n", err);
				++nerr;
			}
		}
	}

	return -nerr;
}

struct nvm_dev* nvm_sblk_attr_dev(struct nvm_sblk *sblk)
{
	return sblk->dev;
}

struct nvm_addr nvm_sblk_attr_bgn(struct nvm_sblk *sblk)
{
	return sblk->bgn;
}

struct nvm_addr nvm_sblk_attr_end(struct nvm_sblk *sblk)
{
	return sblk->end;
}

struct nvm_geo nvm_sblk_attr_geo(struct nvm_sblk *sblk)
{
	return sblk->geo;
}

void nvm_sblk_pr(struct nvm_sblk *sblk)
{
	printf("sblk {\n");
	printf(" "); nvm_dev_pr(sblk->dev);
	printf(" bgn "); nvm_addr_pr(sblk->bgn);
	printf(" end "); nvm_addr_pr(sblk->end);
	printf(" "); nvm_geo_pr(sblk->geo);
	printf("}\n");
}

