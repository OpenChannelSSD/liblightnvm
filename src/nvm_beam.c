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
	struct nvm_vblock *vblock;
	int i;

	for (i = 0; i < beam->nvblocks; i++) {
		vblock = &beam->vblocks[i];
		nvm_vblock_put(vblock, beam->tgt);
	}
}

static void beam_free(struct beam *beam)
{
	beam_buf_free(&beam->w_buffer);
	beam_put_blocks(beam);
	free(beam);
}

static inline int get_npages_block(struct nvm_vblock *vblock)
{
	return vblock->nppas;
}

/* XXX: All block functions assume that block allocation is thread safe */
/* TODO: Allocate blocks dynamically */
static int switch_block(struct beam **beam)
{
	size_t buf_size;
	int sec_size = (*beam)->tgt->dev->fpage.sec_size;
	int ret;

	/* Write buffer for small writes */
	buf_size = get_npages_block(&(*beam)->vblocks[(*beam)->nvblocks] - 1) *
								sec_size;
	if (buf_size != (*beam)->w_buffer.buf_limit) {
		NVM_DEBUG("Allocating write buffer, buf_size(%lu)\n", buf_size);
		free((*beam)->w_buffer.buf);
		ret = posix_memalign(&(*beam)->w_buffer.buf, sec_size,
                                     buf_size);
		if (ret) {
			NVM_DEBUG("FAILED: buf_size(%lu), sec_size(%d)\n",
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

	NVM_DEBUG("SWITCHED: gid(%d), id(%llu), nppas(%d), nvblocks(%d)\n",
                  (*beam)->gid,
                  (*beam)->current_w_vblock->id,
                  (*beam)->current_w_vblock->nppas,
                  (*beam)->nvblocks);

	return 0;
}

static int preallocate_block(struct beam *beam)
{
	struct nvm_vblock *vblock = &beam->vblocks[beam->nvblocks];
	int ret;

	vblock->flags |= NVM_PROV_SPEC_LUN,

	ret = nvm_vblock_gets(vblock, beam->tgt, beam->lun);
	if (ret) {
		NVM_DEBUG("FAILED: gid(%d)\n", beam->gid);
                return ret;
	}

	NVM_DEBUG("SUCCESS: nvblocks(%d), gid(%d), id(%llu), bppa(%llu)\n",
                  beam->nvblocks,
                  beam->gid,
                  beam->vblocks[beam->nvblocks].id,
                  beam->vblocks[beam->nvblocks].bppa);

	vblock->flags &= ~NVM_PROV_SPEC_LUN;
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

static int beam_init(struct beam *beam, int lun, struct nvm_tgt *tgt)
{
	atomic_assign_inc(&beam_guid, &beam->gid);
	beam->lun = lun;
	beam->nvblocks = 0;
	beam->bytes = 0;
	beam->tgt = tgt;
	if (!beam->tgt) {
		NVM_DEBUG("FAILED: !beam->tgt\n");
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
	struct nvm_fpage *fpage = &beam->tgt->dev->fpage;
	size_t sync_len = beam->w_buffer.cursize - beam->w_buffer.cursync;
	size_t synced_bytes;
	size_t disaligned_data = sync_len % fpage->pln_pg_size;
	size_t ppa_off =
		calculate_ppa_off(beam->w_buffer.cursync, fpage->pln_pg_size);
	int npages = sync_len / fpage->pln_pg_size;
	int synced_pages;

	NVM_DEBUG("called with beam(%p), flags(%d)\n", beam, flags);

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
	synced_pages = nvm_vblock_write(beam->current_w_vblock, beam->tgt,
					beam->w_buffer.sync, npages, ppa_off,
					flags);

	if (synced_pages != npages) {
		NVM_DEBUG("FAILED: synced_pages != npages\n");
		return -1;
	}

	/* We might need to take a lock here */
	synced_bytes = synced_pages * fpage->pln_pg_size;
	beam->bytes += synced_bytes;
	beam->w_buffer.cursync += synced_bytes;
	beam->w_buffer.sync += synced_bytes;

	NVM_DEBUG("synced_bytes(%lu), synced_pages(%d)\n",
		  synced_bytes, synced_pages);

	return 0;
}

static void clean_all(void)
{
	struct beam *b, *b_tmp;

	HASH_ITER(hh, beamt, b, b_tmp) {
		HASH_DEL(beamt, b);
		nvm_tgt_close(b->tgt);
		beam_free(b);
	}
}

int nvm_beam_create(const char *tgt_name, int lun, int flags)
{
	struct nvm_tgt *tgt;
	struct beam *beam;
	int ret;

	tgt = nvm_tgt_open(tgt_name, flags);
	if (!tgt) {
		NVM_DEBUG("FAILED: opening target\n");
		return -1;
	}

	beam = malloc(sizeof(struct beam));
	if (!beam) {
		NVM_DEBUG("FAILED: allocating beam\n");
		nvm_tgt_close(tgt);
		return -ENOMEM;
	}

	ret = beam_init(beam, lun, tgt);
	if (ret) {
		NVM_DEBUG("FAILED: initializing beam\n");
		nvm_tgt_close(tgt);
                free(beam);
                return ret;
        }

	HASH_ADD_INT(beamt, gid, beam);
	NVM_DEBUG("beam(%p), beam->gid(%d), tgt(%p), tgt->fd(%d)\n",
		  beam, beam->gid, tgt, tgt->fd);

	return beam->gid;
}

void nvm_beam_destroy(int beam, int flags)
{
	struct beam *b;
	struct nvm_tgt *tgt;

	NVM_DEBUG("beam(%d)\n", beam);

	HASH_FIND_INT(beamt, &beam, b);
	HASH_DEL(beamt, b);

	tgt = b->tgt;
	beam_free(b);
	nvm_tgt_close(tgt);
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
		NVM_DEBUG("FAILED: beam(%d) does not exist\n", beam);
		return -EINVAL;
	}

	NVM_DEBUG("count(%lu), gid(%d), b(%p), beam(%d), cursize(%lu), "
		  "cursync(%lu), buf_limit(%lu)\n",
		  count,
		  b->gid, b,
		  beam,
		  b->w_buffer.cursize,
		  b->w_buffer.cursync,
		  b->w_buffer.buf_limit);

	while (b->w_buffer.cursize + left > b->w_buffer.buf_limit) {
		NVM_DEBUG("Block overflow: cursize(%lu), cursync(%lu)"
			  ", buf_limit(%lu), left(%lu)\n",
			  b->w_buffer.cursize,
			  b->w_buffer.cursync,
			  b->w_buffer.buf_limit,
			  left);
		size_t fits_buf = b->w_buffer.buf_limit - b->w_buffer.cursize;

		ret = preallocate_block(b);
		if (ret) {
			NVM_DEBUG("FAILED: preallocate_block - ret(%d)", ret);
			return ret;
		}

		memcpy(b->w_buffer.mem, buf, fits_buf);
		b->w_buffer.mem += fits_buf;
		b->w_buffer.cursize += fits_buf;
		if (beam_sync(b, FORCE_SYNC)) {
			NVM_DEBUG("FAILED: beam_sync gid(%d)\n", b->gid);
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
		NVM_DEBUG("FAILED: Beam %d does not exits\n", beam);
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
	struct nvm_fpage *fpage;
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
		NVM_DEBUG("FAILED: Beam %d does not exits\n", beam);
		return -EINVAL;
	}

	fpage = &b->tgt->dev->fpage;

	/* TODO: Improve calculations */
	left_pages = ((count + offset % fpage->sec_size) / fpage->sec_size) +
		((((count) % fpage->sec_size) > 0) ? 1 : 0);

	NVM_DEBUG("count(%lu), left_pages(%lu), gid(%d), b(%p), beam(%d)"
		  ", offset(%lu)\n",
		  count, left_pages, b->gid, b, beam, offset);

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
		NVM_DEBUG("FAILED: left_pages*sec_size(%lu), sec_size(%d)\n",
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

		/* TODO: Send bigger I/Os if we have enough data */
		read_pages = nvm_vblock_read(current_r_vblock, b->tgt, reader,
					     pages_to_read, ppa_off, flags);
		if (read_pages != pages_to_read) {
			NVM_DEBUG("FAILED: read_pages != pages_to_read\n");
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

