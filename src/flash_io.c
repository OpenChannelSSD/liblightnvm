/*
 * flash_io - Flash optimized I/O operations
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
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
#include <errno.h>

#include "flash_beam.h"
#include "assert.h"

int flash_write(int tgt, VBLOCK *vblock, const char *buf,
				size_t ppa_off, size_t count,
				int max_pages_write, int page_size)
{
	size_t bppa = vblock->bppa;
	size_t nppas = vblock->nppas;
	size_t current_ppa = bppa + (ppa_off * 16);
	size_t bytes_per_write;
	ssize_t left = count;
	ssize_t written;
	int pages_per_write;
	char *writer = (char*)buf;

	/* TODO: Metadata */

	LNVM_DEBUG("Writing %lu pages to block %llu. ppa_off: %lu, nppas:%lu\n",
					count, vblock->id, ppa_off, nppas);
	assert(count <= nppas - ppa_off);

	int i = 0;
	/* For now we use pwrite. We will have different IO backends */
	while (left > 0) {
		pages_per_write = (left > max_pages_write) ?
						max_pages_write : left;
		bytes_per_write = pages_per_write * page_size;

		LNVM_DEBUG("Write %lu bytes (%d pages) - blk:%llu, ppa:%lu\n",
				bytes_per_write, pages_per_write, vblock->id,
				current_ppa);

		written = pwrite(tgt, writer, bytes_per_write,
						current_ppa * 4096);
		if (written != bytes_per_write) {
			LNVM_DEBUG("Error writing %d pages (%lu bytes) "
					"to ppa: %lu, block %llu (tgt:%d)\n",
					pages_per_write, bytes_per_write,
					current_ppa, vblock->id,
					tgt);
			/* TODO: Retry with another page, and another block */
			return 0;
		}

		writer += bytes_per_write;
		current_ppa += pages_per_write * 16;
		left -= pages_per_write * 16;

		printf("i:%d, left:%d\n", i++, left);
	}

	LNVM_DEBUG("Written %lu pages to block %llu (tgt:%d)",
					count, vblock->id, tgt);

	return count;
}

int flash_read(int tgt, VBLOCK *vblock, void *buf, size_t ppa_off,
			size_t count, int max_pages_read, int page_size)
{
	size_t bppa = vblock->bppa;
	size_t nppas = vblock->nppas;
	size_t current_ppa = bppa + (ppa_off * 16);
	size_t bytes_per_read;
	ssize_t left = count;
	ssize_t read;
	int pages_per_read;
	char *reader = (char*)buf;

	assert(count <= nppas - ppa_off);

	LNVM_DEBUG("Read %lu pages from block %llu (tgt:%d)",
					count, vblock->id, tgt);

	/* For now we use pread. We will have different IO backends */
	while (left > 0) {
		pages_per_read = (left > max_pages_read) ?
						max_pages_read : left;
		bytes_per_read = pages_per_read * page_size;

		LNVM_DEBUG("Read %lu bytes (%d pages) - blk:%llu, ppa:%lu\n",
				bytes_per_read, pages_per_read, vblock->id,
				current_ppa);
retry:
		read = pread(tgt, reader, bytes_per_read,
						current_ppa * 4096);
		if (read != bytes_per_read) {
			if (errno == EINTR)
				goto retry;
			return -1;
		}

		reader += bytes_per_read;
		current_ppa += pages_per_read * 16;
		left -= pages_per_read * 16;
	}

	return count;
}
