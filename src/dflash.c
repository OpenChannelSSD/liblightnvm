/*
 * dflash - user-space append-only file system for flash memories
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

#define _GNU_SOURCE

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <liblightnvm.h>

#include "likely.h"
#include "flash_file.h"

static struct atomic_guid dflash_guid = {
	.guid = 0,
};

static struct atomic_guid fd_guid = {
	.guid = 0,
};

static struct dflash_file *dfilet = NULL;
static struct dflash_fdentry *fdt = NULL;

static void file_init(struct dflash_file *f, uint32_t stream_id, int tgt)
{
	atomic_assign_inc_id(&dflash_guid, &f->gid);
	f->tgt = tgt;
	f->stream_id = stream_id;
	f->nvblocks = 0;
	f->bytes = 0;

	f->w_buffer.buf = NULL;
	f->w_buffer.mem = NULL;
	f->w_buffer.sync = NULL;
	f->w_buffer.buf_limit = 0;
	f->w_buffer.cursize = 0;
	f->w_buffer.cursync = 0;

	f->current_w_vblock = NULL;

	/* TODO: Access times */
}

static void file_buf_free(struct w_buffer *buf)
{
	if (buf->buf) {
		free(buf->buf);
		buf->buf = NULL;
		buf->mem = NULL;
		buf->sync = NULL;
	}
}

static void file_put_blocks(struct dflash_file *f)
{
	int i;

	for(i = 0; i < f->nvblocks; i++) {
		put_block(f->tgt, &f->vblocks[i]);
	}
}

static void file_free(struct dflash_file *f)
{
	file_buf_free(&f->w_buffer);
	free(f);
}

/* XXX: All block functions assume that block allocation is thread safe */
/* TODO: Allocate blocks dynamically */
static int switch_block(struct dflash_file **f)
{
	size_t buf_size;
	int ret;

	/* Write buffer for small writes */
	buf_size = get_npages_block(&(*f)->vblocks[(*f)->nvblocks] - 1) *
								PAGE_SIZE;
	if (buf_size != (*f)->w_buffer.buf_limit) {
		LNVM_DEBUG("Allocating new write buffer of size %lu\n",
								buf_size);
		free((*f)->w_buffer.buf);
		ret = posix_memalign(&(*f)->w_buffer.buf, PAGE_SIZE, buf_size);
		if (ret) {
			LNVM_DEBUG("Cannot allocate memory "
					" (%lu bytes - align. %d)\n",
				buf_size, PAGE_SIZE);
			return ret;
		}
		(*f)->w_buffer.buf_limit = buf_size;
	}

	(*f)->w_buffer.mem = (*f)->w_buffer.buf;
	(*f)->w_buffer.sync = (*f)->w_buffer.buf;
	(*f)->w_buffer.cursize = 0;
	(*f)->w_buffer.cursync = 0;

	(*f)->current_w_vblock = &(*f)->vblocks[(*f)->nvblocks - 1];

	LNVM_DEBUG("Block switched. File: %lu, id: %lu. Total blocks: %d\n",
			(*f)->gid,
			(*f)->current_w_vblock->id,
			(*f)->nvblocks);

	return 0;
}

static int preallocate_block(struct dflash_file *f)
{
	struct vblock *vblock = &f->vblocks[f->nvblocks];
	int ret = 0;

	ret = get_block(f->tgt, f->stream_id, vblock);
	if (ret) {
		LNVM_DEBUG("Could not allocate a new block for file %lu\n",
				f->gid);
		goto out;
	}

	LNVM_DEBUG("Block preallocated (pos:%d). File: %lu, id: %lu, "
			" bppa: %lu\n",
			f->nvblocks,
			f->gid,
			f->vblocks[f->nvblocks].id,
			f->vblocks[f->nvblocks].bppa);

	f->nvblocks++;
out:
	return ret;
}

static int allocate_block(struct dflash_file *f)
{
	int ret;

	ret = preallocate_block(f);
	if (ret)
		goto out;
	ret = switch_block(&f);

out:
	return ret;
}

static int file_sync(int fd, struct dflash_file *f, uint8_t flags)
{
	size_t sync_len = f->w_buffer.cursize - f->w_buffer.cursync;
	size_t ppa_off = calculate_ppa_off(f->w_buffer.cursync);
	size_t disaligned_data = sync_len % PAGE_SIZE;
	size_t synced_bytes;
	uint16_t synced_pages;
	uint16_t npages = sync_len / PAGE_SIZE;

	if (((flags & OPTIONAL_SYNC) && (sync_len < PAGE_SIZE)) ||
		(sync_len == 0))
		return 0;

	if (flags & FORCE_SYNC) {
		if (f->w_buffer.cursync + sync_len == f->w_buffer.buf_limit) {
			/* TODO: Metadata */
		}

		if (disaligned_data > 0) {
			/* TODO: Add padding */
			npages++;
		}
	} else {
		sync_len -= disaligned_data;
	}

	/* write data to media */
	synced_pages = flash_write(f->tgt, f->current_w_vblock, f->w_buffer.sync,
					ppa_off, npages);
	if (synced_pages != npages) {
		LNVM_DEBUG("Error syncing data\n");
		return -1;
	}

	/* We might need to take a lock here */
	synced_bytes = synced_pages * PAGE_SIZE;
	f->bytes += synced_bytes;
	f->w_buffer.cursync += synced_bytes;
	f->w_buffer.sync += synced_bytes;

	LNVM_DEBUG("Synced bytes: %lu (%d pages)\n",
			synced_bytes, synced_pages);

	/* TODO: Access times */
	return 0;
}

static void clean_fd(struct dflash_fdentry *fd_entry)
{
	HASH_DEL(fdt, fd_entry);
	LNVM_DEBUG("Closed fd %lu for file %lu\n",
			fd_entry->fd, fd_entry->dfile->gid);
	/* FIXME: For now, free write buffer here. In the future we might want
	 * to maintain this buffer as a page cache for reads
	 */
	file_buf_free(&fd_entry->dfile->w_buffer);
	free(fd_entry);
}

static void clean_file_fd(uint64_t gid) {
	struct dflash_fdentry *fd_entry, *tmp;

	HASH_ITER(hh, fdt, fd_entry, tmp) {
		if (fd_entry->dfile->gid == gid) {
			clean_fd(fd_entry);
		}
	}
}

static void target_clean(int tgt)
{
	struct dflash_file *f, *tmp;

	/* Assume only one target and that all files belong to that target*/
	HASH_ITER(hh, dfilet, f, tmp) {
		clean_file_fd(f->gid);
		HASH_DEL(dfilet, f);
		file_free(f);
	}

	assert(HASH_COUNT(fdt) == 0);
	assert(HASH_COUNT(dfilet) == 0);
}

/*
 * liblightnvm flash API
 */

int nvm_target_open(const char *tgt, int flags)
{
	char tgt_loc[NVM_TGT_NAME_MAX] = "/dev/";
	int tgt_id;

	strcat(tgt_loc, tgt);
	tgt_id = open(tgt_loc, O_RDWR | O_DIRECT);
	if (tgt_id < 0) {
		LNVM_DEBUG("Failed to open LightNVM target %s (%d)\n",
				tgt_loc, tgt_id);
	}

	return tgt_id;
}

void nvm_target_close(int tgt)
{
	close(tgt);
	target_clean(tgt);
}

int nvm_file_create(int tgt, uint32_t stream_id, int flags)
{
	struct dflash_file *f;

	f = malloc(sizeof(struct dflash_file));
	if (!f)
		return -ENOMEM;

	file_init(f, stream_id, tgt);
	HASH_ADD_INT(dfilet, gid, f);
	LNVM_DEBUG("Created dflash file (p:%p, id:%lu). Target: %d\n",
			f, f->gid, tgt);

	return f->gid;
}

void nvm_file_delete(uint64_t fid, int flags)
{
	struct dflash_file *f;

	LNVM_DEBUG("Deleting file with id %lu\n", fid);

	HASH_FIND_INT(dfilet, &fid, f);
	clean_file_fd(fid);
	HASH_DEL(dfilet, f);
	file_put_blocks(f);
	file_free(f);
}

/*
 * TODO: Assign different file descriptors to same dflash file. For now access
 * dflash files by file id
 */
int nvm_file_open(uint64_t fid, int flags)
{
	struct dflash_file *f;
	struct dflash_fdentry *fd_entry;
	int ret = 0;

	HASH_FIND_INT(dfilet, &fid, f);
	if (!f) {
		LNVM_DEBUG("File with id %lu does not exist\n", fid);
		return -EINVAL;
	}

	if (!f->current_w_vblock) {
		ret = allocate_block(f);
		if (ret)
			goto error;
	}

	fd_entry = malloc(sizeof(struct dflash_fdentry));
	if (!fd_entry) {
		ret = -ENOMEM;
		goto error;
	}

	atomic_assign_inc_id(&fd_guid, &fd_entry->fd);
	fd_entry->dfile = f;
	HASH_ADD_INT(fdt, fd, fd_entry);

	LNVM_DEBUG("Opened fd %lu for file %lu\n", fd_entry->fd, fid);
	return fd_entry->fd;

error:
	free(f->w_buffer.buf);
	return ret;
}

void nvm_file_close(int fd, int flags)
{
	struct dflash_fdentry *fd_entry;
	struct dflash_file *f;

	HASH_FIND_INT(fdt, &fd, fd_entry);
	if (!fd_entry) {
		LNVM_DEBUG("File descriptor %d does not exist\n", fd);
	}
	f = fd_entry->dfile;

	if (file_sync(fd, f, FORCE_SYNC)) {
		LNVM_DEBUG("Cannot force sync for file %lu\n", f->gid);
	}

	clean_fd(fd_entry);
}

/* TODO: Implement a pool of available bloks to support double buffering */
/*
 * TODO: Flush pages in a different thread as write buffer gets filled up,
 * instead of flushing the whole block at a time
 */
size_t nvm_file_append(int fd, const void *buf, size_t count)
{
	struct dflash_fdentry *fd_entry;
	struct dflash_file *f;
	size_t offset = 0;
	size_t left = count;
	char *cache;
	int ret;

	HASH_FIND_INT(fdt, &fd, fd_entry);
	if (!fd_entry) {
		LNVM_DEBUG("File descriptor %d does not exist\n", fd);
		return -EINVAL;
	}
	f = fd_entry->dfile;
	cache = f->w_buffer.mem;

	LNVM_DEBUG("Append %lu bytes to file %lu (p:%p, fd:%d). Csize:%lu, "
			"Csync:%lu, BL:%lu\n",
				count,
				f->gid, f,
				fd,
				f->w_buffer.cursize,
				f->w_buffer.cursync,
				f->w_buffer.buf_limit);

	while (f->w_buffer.cursize + left > f->w_buffer.buf_limit) {
		LNVM_DEBUG("Block overflow. Csize:%lu, Csync:%lu, BL:%lu "
				"left:%lu\n",
				f->w_buffer.cursize,
				f->w_buffer.cursync,
				f->w_buffer.buf_limit,
				left);
		size_t fits_buf = f->w_buffer.buf_limit - f->w_buffer.cursize;
		ret = preallocate_block(f);
		if (ret)
			return ret;
		memcpy(cache, buf, fits_buf);
		cache += fits_buf;
		f->w_buffer.cursize += fits_buf;
		if (file_sync(fd, f, FORCE_SYNC)) {
			LNVM_DEBUG("Cannot force sync for file %lu\n", f->gid);
			return -ENOSPC;
		}
		switch_block(&f);
		cache = f->w_buffer.mem; /* Update cache pointer */

		left -= fits_buf;
		offset += fits_buf;
	}

	cache = f->w_buffer.mem; /* Update cache pointer */
	memcpy(cache, buf + offset, left);
	cache += left;
	f->w_buffer.cursize += left;

	return count;
}

int nvm_file_sync(int fd)
{
	return 0;
}

/*
 * Flag for aligned buffer
 */
size_t nvm_file_read(int fd, void *buf, size_t count, off_t offset, int flags)
{
	struct dflash_fdentry *fd_entry;
	struct dflash_file *f;
	struct vblock *current_r_vblock;
	size_t block_off, ppa_off, page_off;
	size_t ppa_count;
	size_t nppas;
	size_t left_bytes = count;
	/* TODO: Improve this calculations */
	size_t left_pages = count / PAGE_SIZE +
		((((count) % PAGE_SIZE) > 0) ? 1 : 0) +
		(((offset / PAGE_SIZE) != (count + offset) / PAGE_SIZE) ? 1 : 0);
	size_t valid_bytes;
	size_t pages_to_read, bytes_to_read;
	uint16_t read_pages;
	/* char *cache; // Used when trying write cache for reads*/
	void *read_buf;
	char *reader;
	char *writer = buf;
	int ret;

	HASH_FIND_INT(fdt, &fd, fd_entry);
	if (!fd_entry) {
		LNVM_DEBUG("File descriptor %d does not exist\n", fd);
		return -EINVAL;
	}
	f = fd_entry->dfile;
	/* cache = f->w_buffer.mem; */

	LNVM_DEBUG("Read %lu bytes (pgs:%lu) from file %lu (p:%p, fd:%d). Off:%lu\n",
			count, left_pages,
			f->gid, f,
			fd,
			offset);

	/* Assume that all blocks forming the file have same size */
	nppas = get_npages_block(&f->vblocks[0]);

	ppa_count = offset / PAGE_SIZE;
	block_off = ppa_count/ nppas;
	ppa_off = ppa_count % nppas;
	page_off = offset % PAGE_SIZE;

	current_r_vblock = &f->vblocks[block_off];

	pages_to_read = (nppas > left_pages) ? left_pages : nppas;
	ret = posix_memalign(&read_buf, PAGE_SIZE, pages_to_read * PAGE_SIZE);
	if (ret) {
		LNVM_DEBUG("Cannot allocate memory (%lu bytes - align. %d )\n",
				left_pages * PAGE_SIZE, PAGE_SIZE);
		return ret;
	}
	reader = read_buf;

	while (left_bytes) {
		bytes_to_read = pages_to_read * PAGE_SIZE;
		valid_bytes = (left_bytes > bytes_to_read) ?
						bytes_to_read : left_bytes;

		if (UNLIKELY(pages_to_read + ppa_off > nppas)) {
			while (pages_to_read + ppa_off > nppas)
				pages_to_read--;

			valid_bytes = (nppas * PAGE_SIZE) -
					(ppa_off * PAGE_SIZE) - page_off;
		}

		assert(left_bytes <= left_pages * PAGE_SIZE);

		read_pages = flash_read(f->tgt, current_r_vblock, reader,
						ppa_off, pages_to_read);
		if (read_pages != pages_to_read)
			return -1;

		/* TODO: Optional - Flag for aligned memory */
		memcpy(writer, reader + page_off, valid_bytes);
		writer += valid_bytes;

		reader = read_buf;

		block_off++;
		ppa_off = 0;
		page_off = 0;
		current_r_vblock = &f->vblocks[block_off];

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
	pthread_spin_init(&dflash_guid.lock, PTHREAD_PROCESS_SHARED);
	return 0;
}

void nvm_fini()
{
	pthread_spin_destroy(&fd_guid.lock);
	pthread_spin_destroy(&dflash_guid.lock);
}
