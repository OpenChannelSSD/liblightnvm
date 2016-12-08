/*
 * bbt - Bad-block-table functions
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <libudev.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm.h>
#include <nvm_debug.h>

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
}

struct nvm_bbt *nvm_bbt_get(struct nvm_dev *dev, struct nvm_addr addr,
			    struct nvm_return *ret)
{
	struct nvm_passthru_vio ctl;
	struct nvm_bbt *bbt;
	struct krnl_bbt *k_bbt;
	int err, i;

	bbt = malloc(sizeof(*bbt));
	if (!bbt) {
		errno = ENOMEM;
		return NULL;
	}

	bbt->addr = addr;
	bbt->nblks = dev->geo.nblocks * dev->geo.nplanes;
	bbt->blks = nvm_buf_alloc(dev->geo, sizeof(*bbt->blks) * bbt->nblks);
	if (!bbt->blks) {
		free(bbt);
		errno = ENOMEM;
		return NULL;
	}

	k_bbt = nvm_buf_alloc(dev->geo, sizeof(*k_bbt) + \
			      sizeof(*(k_bbt->blk)) * bbt->nblks);
	if (!k_bbt) {
		free(bbt->blks);
		free(bbt);
		errno = ENOMEM;
		return NULL;
	}

	memset(&ctl, 0, sizeof(ctl));	// Setup the IOCTL
	ctl.opcode = S12_OPC_GET_BBT;
	ctl.addr = (uint64_t)k_bbt;
	ctl.data_len = bbt->nblks * sizeof(*bbt->blks);
	ctl.ppa_list = nvm_addr_gen2dev(dev, addr).ppa;
	ctl.nppas = 0;

	err = ioctl(dev->fd, NVME_NVM_IOCTL_ADMIN_VIO, &ctl);
	if (ret) {			// Fill return-codes when available
		ret->result = ctl.result;
		ret->status = ctl.status;
	}
	if (err || (k_bbt->tblks != bbt->nblks)) {
		errno = EIO;
		free(bbt->blks);
		free(bbt);
		return NULL;
	}
	if (!(k_bbt->tblid[0] == 'B' && k_bbt->tblid[1] == 'B' &&
	      k_bbt->tblid[2] == 'L' && k_bbt->tblid[3] == 'T')) {
		errno = EIO;
		free(bbt->blks);
		free(bbt);
		return NULL;
	}

	for (i = 0; i < bbt->nblks; ++i) {
		bbt->blks[i] = k_bbt->blk[i];
	}

	return bbt;
}

int nvm_bbt_set(struct nvm_dev *dev, struct nvm_bbt *bbt,
		struct nvm_return *ret)
{
	// TODO: Setup the IOCTL

	errno = ENOTSUP;
	return -1;
}

void nvm_bbt_pr(struct nvm_bbt *bbt)
{
	int i, nnotgood;

	printf("bbt {\n");
	printf("  addr"); nvm_addr_pr(bbt->addr);
	printf("  nblks(%lu) {", bbt->nblks);
	nnotgood = 0;
	for (i = 0; i < bbt->nblks; ++i) {
		if (bbt->blks[i]) {
			++nnotgood;
			printf("\n    i(%d) = %u", i, bbt->blks[i]);
		}
	}
	printf("\n  }\n");
	printf("  nnotgood(%d)\n", nnotgood);
	printf("}\n");
}

