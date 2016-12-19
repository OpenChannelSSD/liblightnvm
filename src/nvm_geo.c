/*
 * geo - Geometry functions
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
#include <stdio.h>
#include <liblightnvm.h>

void nvm_geo_pr(const struct nvm_geo *geo)
{
	printf("geo {\n");
	printf(" nchannels(%lu), nluns(%lu), nplanes(%lu),\n",
	       geo->nchannels, geo->nluns, geo->nplanes);
	printf(" nblocks(%lu), npages(%lu), nsectors(%lu),\n",
	       geo->nblocks, geo->npages, geo->nsectors);
	printf(" page_nbytes(%lu), sector_nbytes(%lu), meta_nbytes(%lu),\n",
	       geo->page_nbytes, geo->sector_nbytes, geo->meta_nbytes);
	printf(" tbytes(%lub:%luMb),\n",
	       geo->tbytes, geo->tbytes >> 20);
	printf(" vpg_nbytes(%lub:%luKb),\n",
	       geo->vpg_nbytes, geo->vpg_nbytes >> 10);
	printf(" vblk_nbytes(%lub:%luMb)\n",
	       geo->vblk_nbytes, geo->vblk_nbytes >> 20);
	printf("}\n");
}

