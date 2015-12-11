/*
 * flash_append - user-space append-only file system for flash memories
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
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

#include "../util/likely.h"
#include "flash_beam.h"

static struct atomic_cnt beam_guid = {
	.cnt = 0,
};

static struct atomic_cnt fd_guid = {
	.cnt = 0,
};

static struct beam *beamt = NULL;

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
	NVM_PROV prov = {
		.flags = NVM_PROV_SPEC_LUN,
		.lun_status = NULL,
	};
	int i;

	for (i = 0; i < beam->nvblocks; i++) {
		prov.vblock = &beam->vblocks[i];
		nvm_put_block(beam->tgt->tgt_id, &prov);
	}
}

static void beam_free(struct beam *beam)
{
	beam_buf_free(&beam->w_buffer);
	beam_put_blocks(beam);
	free(beam);
}

static inline int get_npages_block(NVM_VBLOCK *vblock)
{
	return vblock->nppas;
}

/* XXX: All block functions assume that block allocation is thread safe */
/* TODO: Allocate blocks dynamically */
static int switch_block(struct beam **beam)
{
	size_t buf_size;
	int sec_size = (*beam)->tgt->tgt->dev->flash_page.sec_size;
	int ret;

	/* Write buffer for small writes */
	buf_size = get_npages_block(&(*beam)->vblocks[(*beam)->nvblocks] - 1) *
								sec_size;
	if (buf_size != (*beam)->w_buffer.buf_limit) {
		LNVM_DEBUG("Allocating new write buffer of size %lu\n",
								buf_size);
		free((*beam)->w_buffer.buf);
		ret = posix_memalign(&(*beam)->w_buffer.buf, sec_size,
								buf_size);
		if (ret) {
			LNVM_DEBUG("Cannot allocate memory "
					" (%lu bytes - align. %d)\n",
				buf_size, sec_size);
			return ret;
		}
		(*beam)->w_buffer.buf_limit = buf_size;
	}

	(*beam)->w_buffer.mem = (*beam)->w_buffer.buf;
	(*beam)->w_buffer.sync = (*beam)->w_buffer.buf;
	(*beam)->w_buffer.cursize = 0;
	(*beam)->w_buffer.cursync = 0;

	(*beam)->current_w_vblock = &(*beam)->vblocks[(*beam)->nvblocks - 1];

	LNVM_DEBUG("Block switched. Beam:%d,id:%llu,nppas:%d;.Total blocks: %d\n",
			(*beam)->gid,
			(*beam)->current_w_vblock->id,
			(*beam)->current_w_vblock->nppas,
			(*beam)->nvblocks);

	return 0;
}

static int preallocate_block(struct beam *beam)
{
	NVM_VBLOCK *vblock = &beam->vblocks[beam->nvblocks];
	NVM_PROV prov = {
		.vblock = vblock,
		.flags = NVM_PROV_SPEC_LUN,
		.lun_status = NULL,
	};
	int ret = 0;

	ret = nvm_get_block(beam->tgt->tgt_id, beam->lun, &prov);
	if (ret) {
		LNVM_DEBUG("Could not allocate a new block for beam %d\n",
				beam->gid);
		goto out;
	}

	LNVM_DEBUG("Block preallocated (pos:%d). Beam: %d, id: %llu, "
			" bppa: %llu\n",
			beam->nvblocks,
			beam->gid,
			beam->vblocks[beam->nvblocks].id,
			beam->vblocks[beam->nvblocks].bppa);

	beam->nvblocks++;
out:
	return ret;
}

static int allocate_block(struct beam *beam)
{
	int ret;

	// TODO: Retry if preallocation fails - when saving metadata
	ret = preallocate_block(beam);
	if (ret)
		goto out;
	ret = switch_block(&beam);

out:
	return ret;
}

static int beam_init(struct beam *beam, int lun, int tgt)
{
	atomic_assign_inc(&beam_guid, &beam->gid);
	beam->lun = lun;
	beam->nvblocks = 0;
	beam->bytes = 0;

	beam->tgt = get_lnvm_tgt_map(tgt);
	if (!beam->tgt) {
		LNVM_DEBUG("Initializing beam on uninitialized target\n");
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
	struct lnvm_fpage *fpage = &beam->tgt->tgt->dev->flash_page;
	size_t sync_len = beam->w_buffer.cursize - beam->w_buffer.cursync;
	size_t synced_bytes;
	size_t disaligned_data = sync_len % fpage->pln_pg_size;
	size_t ppa_off =
		calculate_ppa_off(beam->w_buffer.cursync, fpage->pln_pg_size);
	int npages = sync_len / fpage->pln_pg_size;
	int synced_pages;

	if (((flags & OPTIONAL_SYNC) && (sync_len < fpage->pln_pg_size)) ||
		(sync_len == 0))
		return 0;

	if (flags & FORCE_SYNC) {
		if (beam->w_buffer.cursync + sync_len ==
						beam->w_buffer.buf_limit) {
			/* TODO: Metadata */
		}

		if (disaligned_data > 0) {
			/* Add padding to current page */
			int padding = fpage->pln_pg_size - disaligned_data;
			memset(beam->w_buffer.mem, '\0', padding);
			beam->w_buffer.cursize += padding;
			beam->w_buffer.mem += padding;

			npages++;
		}
	} else {
		sync_len -= disaligned_data;
	}

	/* write data to media */
	synced_pages = flash_write(beam->tgt->tgt_id, beam->current_w_vblock,
				beam->w_buffer.sync, ppa_off, npages, fpage);

	if (synced_pages != npages) {
		LNVM_DEBUG("Error syncing data\n");
		return -1;
	}

	/* We might need to take a lock here */
	synced_bytes = synced_pages * fpage->pln_pg_size;
	beam->bytes += synced_bytes;
	beam->w_buffer.cursync += synced_bytes;
	beam->w_buffer.sync += synced_bytes;

	LNVM_DEBUG("Synced bytes: %lu (%d pages)\n", synced_bytes, synced_pages);

	return 0;
}

static void clean_all()
{
	struct beam *b, *b_tmp;

	HASH_ITER(hh, beamt, b, b_tmp) {
		HASH_DEL(beamt, b);
		nvm_target_close(b->tgt->tgt_id);
		beam_free(b);
	}
}

/*
 * liblightnvm flash API
 */

int nvm_beam_create(const char *tgt, int lun, int flags)
{
	struct beam *beam;
	int tgt_fd;
	int ret;

	tgt_fd = nvm_target_open(tgt, flags);
	if (tgt_fd < 0)
		return tgt_fd;

	beam = malloc(sizeof(struct beam));
	if (!beam)
		return -ENOMEM;

	ret = beam_init(beam, lun, tgt_fd);
	if (ret)
		goto error;

	HASH_ADD_INT(beamt, gid, beam);
	LNVM_DEBUG("Created flash beam (p:%p, id:%d). Target: %d\n",
			beam, beam->gid, tgt_fd);

	return beam->gid;

error:
	free(beam);
	return ret;
}

void nvm_beam_destroy(int beam, int flags)
{
	struct beam *b;
	int tm;

	LNVM_DEBUG("Deleting beam with id %d\n", beam);

	HASH_FIND_INT(beamt, &beam, b);
	HASH_DEL(beamt, b);

	tm = b->tgt->tgt_id;
	nvm_target_close(tm);

	beam_free(b);
}


/* TODO: Implement a pool of available bloks to support double buffering */
/*
 * TODO: Flush pages in a different thread as write buffer gets filled up,
 * instead of flushing the whole block at a time
 */
ssize_t nvm_beam_append(int beam, const void *buf, size_t count)
{
	struct beam *b;
	size_t offset = 0;
	size_t left = count;
	int ret;

	HASH_FIND_INT(beamt, &beam, b);
	if (!b) {
		LNVM_DEBUG("Beam %d does not exits\n", beam);
		return -EINVAL;
	}

	LNVM_DEBUG("Append %lu bytes to beam %d (p:%p, b:%d). Csize:%lu, "
			"Csync:%lu, BL:%lu\n",
				count,
				b->gid, b,
				beam,
				b->w_buffer.cursize,
				b->w_buffer.cursync,
				b->w_buffer.buf_limit);

	while (b->w_buffer.cursize + left > b->w_buffer.buf_limit) {
		LNVM_DEBUG("Block overflow. Csize:%lu, Csync:%lu, BL:%lu "
				"left:%lu\n",
				b->w_buffer.cursize,
				b->w_buffer.cursync,
				b->w_buffer.buf_limit,
				left);
		size_t fits_buf = b->w_buffer.buf_limit - b->w_buffer.cursize;
		ret = preallocate_block(b);
		if (ret)
			return ret;
		memcpy(b->w_buffer.mem, buf, fits_buf);
		b->w_buffer.mem += fits_buf;
		b->w_buffer.cursize += fits_buf;
		if (beam_sync(b, FORCE_SYNC)) {
			LNVM_DEBUG("Cannot force sync for beam %d\n", b->gid);
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
		LNVM_DEBUG("Beam %d does not exits\n", beam);
		return -EINVAL;
	}

	// TODO: Expose flags to application
	return beam_sync(b, FORCE_SYNC);
}

/*
 * Flag for aligned buffer
 */
ssize_t nvm_beam_read(int beam, void *buf, size_t count, off_t offset, int flags)
{
	struct beam *b;
	NVM_VBLOCK *current_r_vblock;
	struct lnvm_fpage *fpage;
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

	HASH_FIND_INT(beamt, &beam, b);
	if (!b) {
		LNVM_DEBUG("Beam %d does not exits\n", beam);
		return -EINVAL;
	}

	fpage = &b->tgt->tgt->dev->flash_page;

	/* TODO: Improve calculations */
	left_pages = ((count + offset % fpage->sec_size) / fpage->sec_size) +
		((((count) % fpage->sec_size) > 0) ? 1 : 0);

	LNVM_DEBUG("Read %lu bytes (pgs:%lu) from beam %d (p:%p, b:%d). Off:%lu\n",
			count, left_pages,
			b->gid, b,
			beam,
			offset);

	/* Assume that all blocks forming the beam have same size */
	nppas = get_npages_block(&b->vblocks[0]);

	ppa_count = offset / fpage->sec_size;
	block_off = ppa_count / nppas;
	ppa_off = ppa_count % nppas;
	page_off = offset % fpage->sec_size;

	current_r_vblock = &b->vblocks[block_off];

	pages_to_read = (nppas > left_pages) ? left_pages : nppas;
	ret = posix_memalign(&read_buf, fpage->sec_size,
						pages_to_read * fpage->sec_size);
	if (ret) {
		LNVM_DEBUG("Cannot allocate memory (%lu bytes - align. %d )\n",
				left_pages * fpage->sec_size, fpage->sec_size);
		return ret;
	}
	reader = read_buf;

	/*
	 * We assume that the device supports reading at a sector granurality
	 * (typically 4KB). If not, we deal with the read in LightNVM in the
	 * kernel.
	 */
	while (left_bytes) {
		bytes_to_read = pages_to_read * fpage->sec_size;
		valid_bytes = (left_bytes > bytes_to_read) ?
						bytes_to_read : left_bytes;

		if (UNLIKELY(pages_to_read + ppa_off > nppas)) {
			while (pages_to_read + ppa_off > nppas)
				pages_to_read--;

			valid_bytes = (nppas * fpage->sec_size) -
					(ppa_off * fpage->sec_size) - page_off;
		}

		assert(left_bytes <= left_pages * fpage->sec_size);

		// TODO: Send bigger I/Os if we have enough data
		read_pages = flash_read(b->tgt->tgt_id, current_r_vblock, reader,
					ppa_off, pages_to_read, fpage);
		if (read_pages != pages_to_read)
			return -1;

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

int nvm_init()
{
	pthread_spin_init(&fd_guid.lock, PTHREAD_PROCESS_SHARED);
	pthread_spin_init(&beam_guid.lock, PTHREAD_PROCESS_SHARED);

	/* TODO: Recover state */
	return 0;
}

void nvm_exit()
{
	pthread_spin_destroy(&fd_guid.lock);
	pthread_spin_destroy(&beam_guid.lock);

	/* TODO: save state */

	clean_all();
}
