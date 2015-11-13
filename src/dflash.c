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

static void file_free(struct dflash_file *f)
{
	int i;

	for(i = 0; i < f->nvblocks; i++) {
		put_block(f->tgt, &f->vblocks[i]);
	}

	file_buf_free(&f->w_buffer);
	free(f);
}

/* XXX: All block functions assume that block allocation is thread safe */
/* TODO: Allocate blocks dynamically */

static int switch_block(struct dflash_file **f)
{
	size_t buf_size;
	int ret;

	(*f)->w_buffer.cursize = 0;
	(*f)->w_buffer.cursync = 0;

	/* Write buffer for small writes */
	buf_size = get_npages_block((*f)->tgt, (*f)->stream_id) * PAGE_SIZE;
	if (buf_size != (*f)->w_buffer.buf_limit) {
		free((*f)->w_buffer.buf);
		ret = posix_memalign(&(*f)->w_buffer.buf, PAGE_SIZE, buf_size);
		if (ret) {
			LNVM_DEBUG("Cannot allocate memory (%lu bytes - align. %d )\n",
				buf_size, PAGE_SIZE);
			return ret;
		}
		(*f)->w_buffer.buf_limit = buf_size;
		(*f)->w_buffer.mem = (*f)->w_buffer.buf;
		(*f)->w_buffer.sync = (*f)->w_buffer.buf;
	}

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

	LNVM_DEBUG("Block preallocated (pos:%d). File: %lu, id: %lu, bppa: %lu\n",
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

/*
 * liblightnvm flash API
 */

int nvm_open_target(const char *tgt, int flags)
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

void nvm_close_target(int tgt_id)
{
	close(tgt_id);
}

int nvm_file_create(int tgt_id, uint32_t stream_id, int flags)
{
	struct dflash_file *f;

	f = malloc(sizeof(struct dflash_file));
	if (!f)
		return -ENOMEM;

	file_init(f, stream_id, tgt_id);
	HASH_ADD_INT(dfilet, gid, f);
	LNVM_DEBUG("Created dflash file (p:%p, id:%lu). Target: %d\n",
			f, f->gid, tgt_id);

	return f->gid;
}

void nvm_file_delete(uint64_t file_id, int flags)
{
	struct dflash_file *f;

	LNVM_DEBUG("Deleting file with id %lu\n", file_id);

	HASH_FIND_INT(dfilet, &file_id, f);
	HASH_DEL(dfilet, f);
	file_free(f);
}

/*
 * TODO: Assign different file descriptors to same dflash file. For now access
 * dflash files by file id
 */
int nvm_file_open(uint64_t file_id, int flags)
{
	struct dflash_file *f;
	struct dflash_fdentry *fd_entry;
	int ret = 0;

	HASH_FIND_INT(dfilet, &file_id, f);
	if (!f) {
		LNVM_DEBUG("File with id %lu does not exist\n", file_id);
		return -EINVAL;
	}

	/*
	 * TODO: Allocate only for writes & check if there is space on the
	 * current block (if any)
	 */
	/* Allocate flash block */
	ret = allocate_block(f);
	if (ret)
		goto error;

	fd_entry = malloc(sizeof(struct dflash_fdentry));
	if (!fd_entry) {
		ret = -ENOMEM;
		goto error;
	}

	atomic_assign_inc_id(&fd_guid, &fd_entry->fd);
	fd_entry->dfile = f;
	HASH_ADD_INT(fdt, fd, fd_entry);

	LNVM_DEBUG("Opened fd %lu for file %lu\n", fd_entry->fd, file_id);
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

	HASH_DEL(fdt, fd_entry);
	LNVM_DEBUG("Closed fd %lu for file %lu\n",
			fd_entry->fd, fd_entry->dfile->gid);
	/* FIXME: For now, free write buffer here. In the future we might want
	 * to maintain this buffer as a page cache for reads
	 */
	file_buf_free(&fd_entry->dfile->w_buffer);
	free(fd_entry);
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
	size_t appended;
	char *cache;
	int ret;

	HASH_FIND_INT(fdt, &fd, fd_entry);
	if (!fd_entry) {
		LNVM_DEBUG("File descriptor %d does not exist\n", fd);
		return -EINVAL;
	}
	f = fd_entry->dfile;
	cache = f->w_buffer.mem;

	LNVM_DEBUG("Append %lu bytes to file %lu (p:%p, fd:%d). Csize:%lu, Csync:%lu, BL:%lu\n",
				count,
				f->gid, f,
				fd,
				f->w_buffer.cursize,
				f->w_buffer.cursync,
				f->w_buffer.buf_limit);

	if (f->w_buffer.cursize + count > f->w_buffer.buf_limit) {
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
		offset = fits_buf;
	}
	appended = count - offset;
	memcpy(cache, buf + offset, appended);
	cache += appended;
	f->w_buffer.cursize += appended;

	return appended;
}

int nvm_file_sync(int fd)
{
	return 0;
}

size_t nvm_file_read(int fd, void *buf, size_t count, off_t offset, int flags)
{
	return 0;
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
