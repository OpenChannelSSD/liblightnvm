/*
 * geo - Geometry functions
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
#include <liblightnvm.h>

void nvm_geo_pr(const struct nvm_geo *geo)
{
	printf("geo:");

	if (!geo) {
		printf(" ~\n");
		return;
	}

	printf("\n");
	printf("  verid: 0x%02X\n", geo->verid);
	switch (geo->verid) {
	case NVM_SPEC_VERID_12:
		printf("  nchannels: %zu\n", geo->nchannels);
		printf("  nluns: %zu\n", geo->nluns);
		printf("  nplanes: %zu\n", geo->nplanes);
		printf("  nblocks: %zu\n", geo->nblocks);
		printf("  npages: %zu\n", geo->npages);
		printf("  nsectors: %zu\n", geo->nsectors);
		printf("  page_nbytes: %zu\n", geo->page_nbytes);
		printf("  sector_nbytes: %zu\n", geo->sector_nbytes);
		printf("  meta_nbytes: %zu\n", geo->meta_nbytes);	
		break;

	case NVM_SPEC_VERID_20:
		printf("  npugrp: %zu\n", geo->npugrp);
		printf("  npunit: %zu\n", geo->npunit);
		printf("  nchunk: %zu\n", geo->nchunk);
		printf("  nsectr: %zu\n", geo->nsectr);
		printf("  nbytes: %zu\n", geo->nbytes);
		printf("  nbytes_oob: %zu\n", geo->nbytes_oob);
		break;
	}

	printf("  tbytes: %zu\n", geo->tbytes);
	printf("  tmbytes: %zu\n", geo->tbytes >> 20);

}
