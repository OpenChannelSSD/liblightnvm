/*
 * beam - Beam abstraction user-space append-only interface for liblightnvm
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
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
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <linux/lightnvm.h>

#include <likely.h>
#include <nvm_beam.h>
#include <nvm_debug.h>

static struct atomic_cnt beam_guid = {
	.cnt = 0,
};

static struct atomic_cnt fd_guid = {
	.cnt = 0,
};

static struct beam *beamt;

static void beam_buf_free(struct w_buffer *buf)
{
	if (buf->buf)
		free(buf->buf);

	buf->buf = NULL;
	buf->mem = NULL;
	buf->sync = NULL;
}

static void beam_put_blocks(struct beam *beam)
{
	struct nvm_vblock *vblock;
	int i;

	for (i = 0; i < beam->nvblocks; i++) {
		vblock = &beam->vblocks[i];
		nvm_vblock_put(vblock);
	}
}

static void beam_free(struct beam *beam)
{
	beam_buf_free(&beam->w_buffer);
	beam_put_blocks(beam);
	free(beam);
}

/* XXX: All block functions assume that block allocation is thread safe */
/* TODO: Allocate blocks dynamically */
static int switch_block(struct beam **beam)
{
	size_t buf_size;
	int ret;

	NVM_GEO geo = nvm_dev_attr_geo((*beam)->dev);

	/* Write buffer for small writes */
	buf_size = nvm_dev_attr_geo((*beam)->dev).vblock_nbytes;
	if (buf_size != (*beam)->w_buffer.buf_limit) {
		free((*beam)->w_buffer.buf);
		ret = posix_memalign(&(*beam)->w_buffer.buf, geo.nbytes,
				     buf_size);
		if (ret) {

			return ret;
		}
		(*beam)->w_buffer.buf_limit = buf_size;
	}

	(*beam)->w_buffer.mem = (*beam)->w_buffer.buf;
	(*beam)->w_buffer.sync = (*beam)->w_buffer.buf;
	(*beam)->w_buffer.cursize = 0;
	(*beam)->w_buffer.cursync = 0;

	(*beam)->current_w_vblock = &(*beam)->vblocks[(*beam)->nvblocks - 1];

	return 0;
}

static int preallocate_block(struct beam *beam)
{
	struct nvm_vblock *vblock = &beam->vblocks[beam->nvblocks];
	int ret;

	ret = nvm_vblock_gets(vblock, beam->dev, 0, beam->lun);
	if (ret) {
		return ret;
	}

	beam->nvblocks++;

	return ret;
}

static int allocate_block(struct beam *beam)
{
	int ret;

	/* TODO: Retry if preallocation fails - when saving metadata */
	ret = preallocate_block(beam);
	if (ret)
		return ret;

	ret = switch_block(&beam);

	return ret;
}

static int beam_init(struct beam *beam, int lun, struct nvm_dev *dev)
{
	atomic_assign_inc(&beam_guid, &beam->gid);
	beam->lun = lun;
	beam->nvblocks = 0;
	beam->bytes = 0;
	beam->dev = dev;
	if (!beam->dev) {
		return -EINVAL;
	}

	beam->w_buffer.buf_limit = 0;
	beam->w_buffer.buf = NULL;

	return allocate_block(beam);
}

/*
 * Sync write buffer to the device. The write granurality is determined by the
 * size of the full page size, across flash planes. These sizes are device
 * specific and are queried for each operating device.
 */
static int beam_sync(struct beam *beam, int flags)
{
	size_t vpage_nbytes = nvm_dev_attr_vpage_nbytes(beam->dev);
	size_t sync_len = beam->w_buffer.cursize - beam->w_buffer.cursync;
	size_t disaligned_data = sync_len % vpage_nbytes;
	size_t ppa_off = calculate_ppa_off(beam->w_buffer.cursync, vpage_nbytes);
	int npages = sync_len / vpage_nbytes;
	int synced_pages;
	size_t synced_bytes;


	if (((flags & OPTIONAL_SYNC) && (sync_len < vpage_nbytes)) ||
		(sync_len == 0))
		return 0;

	if (flags & FORCE_SYNC) {
		if (beam->w_buffer.cursync + sync_len ==
						beam->w_buffer.buf_limit) {
			/* TODO: Metadata */
		}

		if (disaligned_data > 0) {
			/* Add padding to current page */
			int padding = vpage_nbytes - disaligned_data;

			memset(beam->w_buffer.mem, '\0', padding);
			beam->w_buffer.cursize += padding;
			beam->w_buffer.mem += padding;

			npages++;
		}
	} else {
		sync_len -= disaligned_data;
	}

	/* write data to media */
	synced_pages = nvm_vblock_pwrite(beam->current_w_vblock,
                                         beam->w_buffer.sync, npages, ppa_off);

	if (synced_pages != npages) {
		return -1;
	}

	/* We might need to take a lock here */
	synced_bytes = synced_pages * vpage_nbytes;
	beam->bytes += synced_bytes;
	beam->w_buffer.cursync += synced_bytes;
	beam->w_buffer.sync += synced_bytes;

	return 0;
}

static void clean_all(void)
{
	struct beam *b, *b_tmp;

	HASH_ITER(hh, beamt, b, b_tmp) {
		HASH_DEL(beamt, b);
		nvm_dev_close(b->dev);
		beam_free(b);
	}
}

int nvm_beam_create(const char *dev_name, int lun, int flags)
{
	struct nvm_dev *dev;
	struct beam *beam;
	int ret;

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		return -1;
	}

	beam = malloc(sizeof(struct beam));
	if (!beam) {
		nvm_dev_close(dev);
		return -ENOMEM;
	}

	ret = beam_init(beam, lun, dev);
	if (ret) {
		nvm_dev_close(dev);
		free(beam);
		return ret;
        }

	HASH_ADD_INT(beamt, gid, beam);

	return beam->gid;
}

void nvm_beam_destroy(int beam, int flags)
{
	struct beam *b;
	struct nvm_dev *dev;

	HASH_FIND_INT(beamt, &beam, b);
	HASH_DEL(beamt, b);

	dev = b->dev;
	beam_free(b);
	nvm_dev_close(dev);
}

/* TODO: Implement a pool of available blocks to support double buffering */
/*
 * TODO: Flush pages in a different thread as write buffer gets filled up,
 *       instead of flushing the whole block at a time
 */
ssize_t nvm_beam_append(int beam, const void *buf, size_t count)
{
	struct beam *b;
	size_t offset = 0;
	size_t left = count;
	int ret;

	HASH_FIND_INT(beamt, &beam, b);
	if (!b) {
		return -EINVAL;
	}

	while (b->w_buffer.cursize + left > b->w_buffer.buf_limit) {

		size_t fits_buf = b->w_buffer.buf_limit - b->w_buffer.cursize;

		ret = preallocate_block(b);
		if (ret) {
			return ret;
		}

		memcpy(b->w_buffer.mem, buf, fits_buf);
		b->w_buffer.mem += fits_buf;
		b->w_buffer.cursize += fits_buf;
		if (beam_sync(b, FORCE_SYNC)) {
			return -ENOSPC;
		}

		switch_block(&b);

		left -= fits_buf;
		offset += fits_buf;
	}

	memcpy(b->w_buffer.mem, buf + offset, left);
	b->w_buffer.mem += left;
	b->w_buffer.cursize += left;

	return count;
}

int nvm_beam_sync(int beam, int flags)
{
	struct beam *b;

	HASH_FIND_INT(beamt, &beam, b);
	if (!b) {
		return -EINVAL;
	}

	/* TODO: Expose flags to application */
	return beam_sync(b, FORCE_SYNC);
}

/*
 * Flag for aligned buffer
 */
ssize_t nvm_beam_read(int beam, void *buf, size_t count, off_t offset,
		      int flags)
{
	struct beam *b;
	struct nvm_vblock *current_r_vblock;
	size_t block_off, ppa_off, page_off;
	size_t ppa_count;
	size_t nppas;
	size_t left_bytes = count;
	size_t valid_bytes;
	size_t left_pages;
	size_t pages_to_read, bytes_to_read;
	void *read_buf;
	char *reader;
	char *writer = buf;
	/* char *cache; // Used when trying write cache for reads*/
	int read_pages;
	int ret;

	NVM_GEO geo;

	HASH_FIND_INT(beamt, &beam, b);
	if (!b) {
		return -EINVAL;
	}

	geo = nvm_dev_attr_geo(b->dev);

	/* TODO: Improve calculations */
	left_pages = ((count + offset % geo.nbytes) / geo.nbytes) +
		((((count) % geo.nbytes) > 0) ? 1 : 0);

	/* Assume that all blocks forming the beam have same size */
	nppas = geo.vblock_nbytes;

	ppa_count = offset / geo.nbytes;
	block_off = ppa_count / nppas;
	ppa_off = ppa_count % nppas;
	page_off = offset % geo.nbytes;

	current_r_vblock = &b->vblocks[block_off];

	pages_to_read = (nppas > left_pages) ? left_pages : nppas;
	ret = posix_memalign(&read_buf, geo.nbytes,
			     pages_to_read * geo.nbytes);
	if (ret) {
		return ret;
	}
	reader = read_buf;

	/*
	 * We assume that the device supports reading at a sector granurality
	 * (typically 4KB). If not, we deal with the read in LightNVM in the
	 * kernel.
	 */
	while (left_bytes) {
		bytes_to_read = pages_to_read * geo.nbytes;
		valid_bytes = (left_bytes > bytes_to_read) ?
						bytes_to_read : left_bytes;

		if (UNLIKELY(pages_to_read + ppa_off > nppas)) {
			while (pages_to_read + ppa_off > nppas)
				pages_to_read--;

			valid_bytes = (nppas * geo.nbytes) -
					(ppa_off * geo.nbytes) - page_off;
		}

		assert(left_bytes <= left_pages * geo.nbytes);

		/* TODO: Send bigger I/Os if we have enough data */
		read_pages = nvm_vblock_pread(current_r_vblock, reader,
                                              pages_to_read, ppa_off);
		if (read_pages != pages_to_read) {
			return -1;
		}

		/* TODO: Optional - Flag for aligned memory */
		memcpy(writer, reader + page_off, valid_bytes);
		writer += valid_bytes;

		reader = read_buf;

		block_off++;
		ppa_off = 0;
		page_off = 0;
		current_r_vblock = &b->vblocks[block_off];

		left_pages -= read_pages;
		left_bytes -= valid_bytes;

		pages_to_read = (nppas > left_pages) ? left_pages : nppas;
	}

	/* TODO: Optional - Flag for aligned memory */
	free(read_buf);
	return count;
}

int nvm_beam_init(void)
{
	pthread_spin_init(&fd_guid.lock, PTHREAD_PROCESS_SHARED);
	pthread_spin_init(&beam_guid.lock, PTHREAD_PROCESS_SHARED);

	/* TODO: Recover state */
	return 0;
}

void nvm_beam_exit(void)
{
	pthread_spin_destroy(&fd_guid.lock);
	pthread_spin_destroy(&beam_guid.lock);

	/* TODO: save state */

	clean_all();
}

