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
	size_t alignment = 0;

	switch(geo->verid) {
	case NVM_SPEC_VERID_12:
		alignment = geo->sector_nbytes;
		break;
	case NVM_SPEC_VERID_20:
		alignment = geo->l.nbytes;
		break;
	}

	if (!nbytes) {
		NVM_DEBUG("!nbytes: %zu", nbytes);
		errno = EINVAL;
		return NULL;
	}
#ifdef WIN32
	buf = _aligned_malloc(nbytes, alignment);
#else
	buf = aligned_alloc(alignment, nbytes);
#endif
	if (!buf) {
		NVM_DEBUG("call to alloc failed");
		return NULL;	// Propagate errno
	}

	return buf;
}

void *nvm_buf_alloca(size_t alignment, size_t nbytes)
{
	char *buf;

	if (!nbytes) {
		errno = EINVAL;
		return NULL;
	}
#ifdef WIN32
	buf = _aligned_malloc(nbytes, alignment);
#else
	buf = aligned_alloc(alignment, nbytes);
#endif
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

void nvm_buf_free(void *buf)
{
#ifdef WIN32
	_aligned_free(buf);
#else
	free(buf);
#endif
}

void nvm_buf_pr(char *buf, size_t nbytes)
{
	const int width = 32;

	// TODO: YAML
	printf("# NVM_BUF_PR - BEGIN **\n");
	for (size_t i = 0; i < nbytes; i++) {
		if (!(i % width))
			printf("\ni[%zu,%zu]: ", i, i + (width - 1));
		printf(" %c", buf[i]);
	}
	printf("# NVM_BUF_PR - END\n");
}


size_t nvm_buf_diff(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	for (size_t i = 0; i < nbytes; ++i)
		if (expected[i] != actual[i])
			++diff;

	return diff;
}

void nvm_buf_diff_pr(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	printf("buf_diffs:\n");
	for (size_t i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			++diff;
			printf(" - {i: '%06lu', exp: '%c', actd: %02d, acth: 0x%02x, actc: %c)\n",
				i, expected[i], (int)actual[i], (int)actual[i],
				(actual[i] > 31 && actual[i] < 127) ? actual[i] : '?');
		}
	}
	printf("buf_nbytes: %zu\n", nbytes);
	printf("buf_nbytes_diff: %zu\n", diff);
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

void nvm_buf_set_pr(struct nvm_buf_set *bufs)
{
	if (!bufs) {
		printf("buf_set: ~\n");
		return;
	}

	printf("buf_set:\n");
	printf("  bufs: %p\n", (void*)bufs);
	printf("  nbytes: %zu\n", bufs->nbytes);
	printf("  nbytes_meta: %zu\n", bufs->nbytes_meta);
	printf("  write: %p\n", (void*)bufs->write);
	printf("  write_meta: %p\n", (void*)bufs->write_meta);
	printf("  read: %p\n",(void*) bufs->read);
	printf("  read_meta: %p\n", (void*)bufs->read_meta);
}

void nvm_buf_set_free(struct nvm_buf_set *bufs)
{
	if (!bufs) {
		NVM_DEBUG("nothing to do, bufs: %p", (void*)bufs);
		return;
	}

	nvm_buf_free(bufs->write);
	nvm_buf_free(bufs->write_meta);
	nvm_buf_free(bufs->read);
	nvm_buf_free(bufs->read_meta);
	nvm_buf_free(bufs);
}

void nvm_buf_set_fill(struct nvm_buf_set *bufs)
{
	if (bufs->nbytes) {
		nvm_buf_fill(bufs->write, bufs->nbytes);
		memset(bufs->read, 0, bufs->nbytes);
	}

	if (bufs->nbytes_meta) {
		nvm_buf_fill(bufs->write_meta, bufs->nbytes_meta);
		memset(bufs->read_meta, 0, bufs->nbytes_meta);
	}
}

struct nvm_buf_set *nvm_buf_set_alloc(struct nvm_dev *dev, size_t nbytes,
				      size_t nbytes_meta)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	struct nvm_buf_set *bufs = NULL;

	bufs = nvm_buf_alloc(geo, sizeof(*bufs));
	if (!bufs) {
		NVM_DEBUG("FAILED: allocating bufs");
		return NULL;
	}
	memset(bufs, 0, sizeof(*bufs));

	if (nbytes) {
		bufs->nbytes = nbytes;

		bufs->write = nvm_buf_alloc(geo, bufs->nbytes);
		if (!bufs->write) {
			NVM_DEBUG("FAILED: alloc w nbytes: %zu", bufs->nbytes);
			nvm_buf_set_free(bufs);
			return NULL;
		}

		bufs->read = nvm_buf_alloc(geo, bufs->nbytes);
		if (!bufs->read) {
			NVM_DEBUG("FAILED: alloc r nbytes: %zu", bufs->nbytes);
			nvm_buf_set_free(bufs);
			return NULL;
		}
	}

	if (nbytes_meta) {
		bufs->nbytes_meta = nbytes_meta;

		bufs->write_meta = nvm_buf_alloc(geo, bufs->nbytes_meta);
		if (!bufs->write_meta) {
			NVM_DEBUG("FAILED: alloc wm nbytes_meta: %zu", bufs->nbytes_meta);
			nvm_buf_set_free(bufs);
			return NULL;
		}

		bufs->read_meta = nvm_buf_alloc(geo, bufs->nbytes_meta);
		if (!bufs->read_meta) {
			NVM_DEBUG("FAILED: alloc rm nbytes_meta: %zu", bufs->nbytes_meta);
			nvm_buf_set_free(bufs);
			return NULL;
		}
	}

	return bufs;
}
