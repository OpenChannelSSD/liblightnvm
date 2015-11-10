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
#include <liblightnvm.h>

#include "dflash.h"

static struct atomic_guid dflash_guid = {
	.guid = 0,
};

static struct atomic_guid fd_guid = {
	.guid = 0,
};

static struct dflash_file *dfilet = NULL;
static struct dflash_fdentry *fdt = NULL;

/* TODO: Map lnvm targets */
/*
 * This function allows an application to choose the LightNVM target from which
 * the new dflash file will obtain flash blocks. If this function is not called
 * before opening a dflash file, a default LightNVM target will be used
 */
uint64_t nvm_create(const char *tgt, uint32_t stream_id, int flags)
{
	struct dflash_file *f;
	char tgt_loc[NVM_TGT_NAME_MAX] = "/dev/";
	int fd;

	f = malloc(sizeof(struct dflash_file));
	if (!f)
		return -ENOMEM;

	strcat(tgt_loc, tgt);
	fd = open(tgt_loc, O_RDWR | O_DIRECT);
	if (fd < 0) {
		LNVM_DEBUG("Failed to open LightNVM target %s (%d)\n",
				tgt_loc, fd);
		return fd;
	}

	f->stream_id = stream_id;
	f->tgt = fd;
	atomic_assign_inc_id(&dflash_guid, &f->gid);
	HASH_ADD_INT(dfilet, gid, f);
	LNVM_DEBUG("Created dflash file. Target: %s\n", tgt);

	return f->gid;
}

void nvm_delete(uint64_t file_id, int flags)
{
	struct dflash_file *f;

	LNVM_DEBUG("Deleting file with id %lu\n", file_id);

	HASH_FIND_INT(dfilet, &file_id, f);
	HASH_DEL(dfilet, f);
	free(f);
}

/* TODO: Assign different file descriptors to same dflash file. For now access
 * dflash files by file id */
int nvm_open(uint64_t file_id, int flags)
{
	struct dflash_file *f;
	struct dflash_fdentry *fd_entry;
	size_t w_buf_size;
	int ret = 0;

	HASH_FIND_INT(dfilet, &file_id, f);
	if (!f) {
		LNVM_DEBUG("File with id %lu does not exist\n", file_id);
		return -EINVAL;
	}

	/* Write buffer for small writes */
	w_buf_size = get_npages_block(f->tgt, f->stream_id) * PAGE_SIZE;
	ret = posix_memalign(&f->w_buffer.buf, PAGE_SIZE, w_buf_size);
	if (ret) {
		LNVM_DEBUG("Cannot allocate memory (%lu bytes - align. %d )\n",
				w_buf_size, PAGE_SIZE);
		return ret;
	}

	f->w_buffer.cursize = 0;
	f->w_buffer.curflush =0;
	f->w_buffer.mem = f->w_buffer.buf;
	f->w_buffer.flush = f->w_buffer.buf;

	fd_entry = malloc(sizeof(struct dflash_fdentry));
	if (!fd_entry) {
		ret = -ENOMEM;
		goto error;
	}

	atomic_assign_inc_id(&fd_guid, &fd_entry->fd);
	fd_entry->dfile = f;
	HASH_ADD_INT(fdt, fd, fd_entry);

	LNVM_DEBUG("Opened fd %d for file %lu\n", fd_entry->fd, file_id);
	return fd_entry->fd;

error:
	free(f->w_buffer.buf);
	return ret;
}

void nvm_close(int fd, int flags)
{
	struct dflash_fdentry *fd_entry;

	HASH_FIND_INT(fdt, &fd, fd_entry);
	HASH_DEL(fdt, fd_entry);
	LNVM_DEBUG("Closed fd %d for file %lu\n",
			fd_entry->fd, fd_entry->dfile->gid);
	/* FIXME: For now, free write buffer here. In the future we might want
	 * to maintain this buffer as a page cache for reads
	 */
	free (fd_entry->dfile->w_buffer.buf);
	free(fd_entry);
}

int nvm_append(int fd, const void *buf, size_t count)
{
	struct dflash_fdentry *fd_entry;
	struct dflash_file *f;

	HASH_FIND_INT(fdt, &fd, fd_entry);
	if (!fd_entry) {
		LNVM_DEBUG("File descriptor %d does not exist\n", fd);
		return -EINVAL;
	}
	f = fd_entry->dfile;

	LNVM_DEBUG("Append to file %lu (fd: %d)\n", f->gid, fd);
	
	return 0;
}

int nvm_read(int fd, void *buf, size_t count, off_t offset, int flags)
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
