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
#ifndef __DFLASH_H
#define __DFLASH_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "provisioning.h"
#include "../util/uthash.h"
#include "../util/debug.h"

/*
 * TODO:
 *	- Dynamic block list
 */

#define NVM_TGT_NAME_MAX NVM_TTYPE_MAX + 5	/* 5 = strlen(/dev/) */

#define MAX_BLOCKS 5
#define PAGE_SIZE 4096

struct atomic_guid {
	uint64_t guid;
	pthread_spinlock_t lock;
};

struct w_buffer {
	size_t cursize;		/* Current buf lenght. Follows mem */
	size_t curflush;	/* Bytes in buf that have been flushed */
	size_t buf_limit;	/* Limit for the allocated memory region */
	void *buf;		/* Buffer to cache writes */
	char *mem;		/* Points to the place in buf where writes can be
				 * appended to. It defines the part of the
				 * buffer containing valid data */
	char *flush;		/* Points to the place in buf until which data
				 * has been flushed to the media */
};

/* TODO: Allocate dynamic number of blocks */
/* TODO: Store the lnvm target the file belongs to */
struct dflash_file {
	uint64_t gid;				/* internal global identifier */
	uint32_t tgt;				/* fd of LightNVM target */
	uint32_t stream_id;			/* stream associated with file */
	struct vblock vblocks[MAX_BLOCKS];	/* vblocks forming the file */
	uint8_t nvblocks;			/* number of vblocks */
	struct w_buffer w_buffer;		/* write buffer */
	unsigned long bytes;			/* valid bytes */
	struct timespec atime;			/* last access time */
	struct timespec mtime;			/* last modify time */
	struct timespec ctime;			/* last change time */
	UT_hash_handle hh;			/* hash handle for uthash */
};

struct dflash_fdentry {
	// uint32_t max_fds;		#<{(| Max number of fds |)}>#
	uint64_t fd;			/* File descriptor */
	struct dflash_file *dfile;	/* DFlash file associate with the fd */
	UT_hash_handle hh;		/* hash handle for uthash */
};

static inline void atomic_assign_inc_id(struct atomic_guid *cnt, uint64_t *id)
{
	pthread_spin_lock(&cnt->lock);
	cnt->guid++;
	*id = cnt->guid;
	pthread_spin_unlock(&cnt->lock);
}

#endif
