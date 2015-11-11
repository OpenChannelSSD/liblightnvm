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
#include "flash_file.h"
#include "assert.h"

uint16_t flash_write(int fd, struct vblock *vblock, const char *buf,
			size_t ppa_off, uint16_t npages)
{
	size_t bppa = vblock->bppa;
	size_t nppas = vblock->nppas;
	size_t current_ppa = bppa + ppa_off;
	size_t left = npages;
	ssize_t written;
	uint8_t max_pages_write = 1; /* Get from NVM_DEV_MAX_SEC */
	uint8_t pages_per_write;
	char *writer = (char*)buf;

	/* TODO: Metadata */

	assert(npages <= nppas - ppa_off);

	LNVM_DEBUG("Syncing fd:%d\n", fd);

	/* For now we use pwrite. We will have different IO backends */
	while (left > 0) {
		pages_per_write = (left > max_pages_write) ?
						max_pages_write : left;

		written = pwrite(fd, writer, pages_per_write * PAGE_SIZE,
					current_ppa * PAGE_SIZE);
		if (written != pages_per_write * PAGE_SIZE) {
			LNVM_DEBUG("Error writing %d pages to block %lu (fd:%d)\n",
					pages_per_write,
					vblock->id,
					fd);
			/* TODO: Retry with another page, and another block */
			return 0;
		}

		writer += pages_per_write;
		current_ppa += pages_per_write;
		left -= pages_per_write;
	}

	LNVM_DEBUG("Written %d pages to block %lu (fd:%d)",
					pages_per_write,
					vblock->id,
					fd);

	return npages;
}
