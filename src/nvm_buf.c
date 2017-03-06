/*
 * buf - Helper functions for buffer management
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>

void *nvm_buf_alloc(const struct nvm_geo *geo, size_t nbytes)
{
	char *buf;

	if (!nbytes) {
		errno = EINVAL;
		return NULL;
	}

	buf = aligned_alloc(geo->sector_nbytes, nbytes);
	if (!buf)
		return NULL;	// Propagate errno

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

	printf("** NVM_BUF_PR - BEGIN **");
	for (size_t i = 0; i < nbytes; i++) {
		if (!(i % width))
			printf("\ni[%lu,%lu]: ", i, i+(width-1));
		printf(" %c", buf[i]);
	}
	printf("\n** NVM_BUF_PR - END **\n");
}

int nvm_buf_from_file(char *buf, size_t nbytes, const char *path)
{
	FILE *fhandle = NULL;
	size_t fcount = 0;

	fhandle = fopen(path, "rb");
	fcount = fread(buf, 1, nbytes, fhandle);
	fclose(fhandle);

	if (fcount != nbytes) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}

int nvm_buf_to_file(char *buf, size_t nbytes, const char *path)
{
	FILE *fhandle = NULL;
	size_t fcount = 0;

	fhandle = fopen(path, "wb");
	fcount = fwrite(buf, 1, nbytes, fhandle);
	fclose(fhandle);

	if (fcount != nbytes) {
		errno = EINVAL;
		return -1;
	}

	return 0;
}
