/*
 * buf - Helper functions for buffer management
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
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_debug.h>

void *nvm_buf_alloc(struct nvm_geo geo, size_t nbytes)
{
	char *buf;
	int ret;

	if (!nbytes) {
		errno = EINVAL;
		return NULL;
	}

	ret = posix_memalign((void **)&buf, geo.sector_nbytes, nbytes);
	if (ret) {
		errno = ret;
		return NULL;
	}

	return buf;
}

void nvm_buf_fill(char *buf, size_t nbytes)
{
	#pragma omp parallel for schedule(static, 1)
	for (size_t i = 0; i < nbytes; ++i)
		buf[i] = (i % 26) + 65;
}

void nvm_buf_pr(char *buf, size_t nbytes)
{
	const int width = 32;
	int i;

	printf("** NVM_BUF_PR - BEGIN **");
	for (i = 0; i < nbytes; i++) {
		if (!(i % width))
			printf("\ni[%d,%d]: ", i, i+(width-1));
		printf(" %c", buf[i]);
	}
	printf("\n** NVM_BUF_PR - END **\n");
}

