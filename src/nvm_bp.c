/*
 * nvm_bp
 *
 * Copyright (C) Simon A. F. Lund <slund@cnexlabs.com>
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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_debug.h>

void nvm_bp_pr(const struct nvm_bp *bp)
{
	if (!bp) {
		printf("nvm_bp: ~\n");
		return;
	}

	printf("nvm_bp: { ws_opt: %zu }\n", bp->ws_opt);

	nvm_dev_pr(bp->dev);
	nvm_geo_pr(bp->geo);
	nvm_buf_set_pr(bp->bufs);
	nvm_addr_prn(bp->addrs, bp->naddrs, bp->dev);
	nvm_vblk_pr(bp->vblk);
}

void nvm_bp_term(struct nvm_bp *bp)
{
	if (!bp) {
		return;
	}

	nvm_buf_set_free(bp->bufs);
	nvm_vblk_free(bp->vblk);
	free(bp);
}

/**
 * Initialize boiler-plate struct
 */
struct nvm_bp *nvm_bp_init(const char *dev_ident, int be_id, int naddrs)
{
	struct nvm_bp *bp;
	int err;

	NVM_DEBUG("dev_ident: %s, be_id: %d, naddrs: %d", dev_ident, be_id,
		  naddrs);

	bp = calloc(1, sizeof(*bp) + sizeof(struct nvm_addr) * naddrs);
	if (!bp) {
		perror("calloc");
		return NULL;
	}

	bp->naddrs = naddrs;
	bp->dev = nvm_dev_openf(dev_ident, be_id);
	if (!bp->dev) {
		perror("nvm_dev_openf errno");
		nvm_bp_term(bp);
		return NULL;
	}
	bp->geo = nvm_dev_get_geo(bp->dev);
	bp->ws_opt = nvm_dev_get_ws_opt(bp->dev);

	err = nvm_cmd_rprt_arbs(bp->dev, NVM_CHUNK_STATE_FREE, bp->naddrs,
				bp->addrs);
	if (err) {
		perror("nvm_cmd_rprt_arbs");
		nvm_bp_term(bp);
		return NULL;
	}

	bp->vblk = nvm_vblk_alloc(bp->dev, bp->addrs, bp->naddrs);
	if (!bp->vblk) {
		perror("nvm_vblk_alloc");
		nvm_bp_term(bp);
		return NULL;
	}

	bp->bufs = nvm_buf_set_alloc(bp->dev, nvm_vblk_get_nbytes(bp->vblk), 0);
	if (!bp->bufs) {
		perror("nvm_buf_set_alloc");
		nvm_bp_term(bp);
		return NULL;
	}
	nvm_buf_set_fill(bp->bufs);

	return bp;
}

struct nvm_bp *nvm_bp_init_from_args(int argc, char **argv)
{
	char dev_ident[NVM_DEV_PATH_LEN+1] = { 0 };
	int be_id = NVM_BE_ANY;
	int naddrs = 1;

	switch(argc) {
	case 4:
		naddrs = strtol(argv[3], NULL, 10);
		/* FALLTHRU */
	case 3:		// Backend identifier
		be_id = strtol(argv[2], NULL, 16);
		/* FALLTHRU */
	case 2:		// Device identifier
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			errno = EINVAL;
			return NULL;
                }
		strncpy(dev_ident, argv[1], NVM_DEV_PATH_LEN);
		break;

	default:
		errno = EINVAL;
		perror("invalid argc");
		return NULL;
	}

	return nvm_bp_init(dev_ident, be_id, naddrs);
}

