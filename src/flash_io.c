/*
 * flash_io - Flash optimized I/O operations
 *
 * Copyright (C) 2015 Javier Gonzalez <javier@cnexlabs.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */
#include <unistd.h>
#include <errno.h>

#include "flash_beam.h"
#include "assert.h"

int flash_write(int tgt, VBLOCK *vblock, const char *buf,
				size_t ppa_off, size_t count,
				int max_pages_write, int write_page_size)
{
	size_t bppa = vblock->bppa;
	size_t nppas = vblock->nppas;
	size_t current_ppa = bppa + ppa_off;
	size_t left = count;
	size_t bytes_per_write;
	ssize_t written;
	int pages_per_write;
	char *writer = (char*)buf;

	/* TODO: Metadata */

	LNVM_DEBUG("Writing %lu pages to block %lu. ppa_off: %lu, nppas:%lu\n",
					count, vblock->id, ppa_off, nppas);
	assert(count <= nppas - ppa_off);

	/* For now we use pwrite. We will have different IO backends */
	while (left > 0) {
		pages_per_write = (left > max_pages_write) ?
						max_pages_write : left;
		bytes_per_write = pages_per_write * write_page_size;

		written = pwrite(tgt, writer, bytes_per_write,
					current_ppa * write_page_size);
		if (written != bytes_per_write) {
			LNVM_DEBUG("Error writing %d pages (%lu bytes) "
					"to ppa: %lu, block %lu (tgt:%d)\n",
					pages_per_write, bytes_per_write,
					current_ppa, vblock->id,
					tgt);
			/* TODO: Retry with another page, and another block */
			return 0;
		}

		writer += bytes_per_write;
		current_ppa += pages_per_write;
		left -= pages_per_write;
	}

	LNVM_DEBUG("Written %lu pages to block %lu (tgt:%d)",
					count, vblock->id, tgt);

	return count;
}

int flash_read(int tgt, VBLOCK *vblock, void *buf, size_t ppa_off,
			size_t count, int max_pages_read, int dev_page_size)
{
	size_t bppa = vblock->bppa;
	size_t nppas = vblock->nppas;
	size_t current_ppa = bppa + ppa_off;
	size_t left = count;
	size_t bytes_per_read;
	ssize_t read;
	int pages_per_read;
	char *reader = (char*)buf;

	assert(count <= nppas - ppa_off);

	LNVM_DEBUG("Read %lu pages from block %lu (tgt:%d)",
					count, vblock->id, tgt);

	/* For now we use pread. We will have different IO backends */
	while (left > 0) {
		pages_per_read = (left > max_pages_read) ?
						max_pages_read : left;
		bytes_per_read = pages_per_read * dev_page_size;

		/* LNVM_DEBUG("Read %lu bytes (%d pages) - blk:%lu, ppa:%lu\n", */
				/* bytes_per_read, pages_per_read, vblock->id, */
				/* current_ppa); */
retry:
		read = pread(tgt, reader, bytes_per_read,
					current_ppa * dev_page_size);
		if (read != bytes_per_read) {
			if (errno == EINTR)
				goto retry;
			return -1;
		}

		reader += bytes_per_read;
		current_ppa += pages_per_read;
		left -= pages_per_read;
	}

	return count;
}
