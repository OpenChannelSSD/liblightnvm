/*
 * flash_beam - I/O abstraction for flash memories.
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
#ifndef __FLASH_BEAM_H
#define __FLASH_BEAM_H

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

#define NVM_TGT_NAME_MAX DISK_NAME_LEN + 5	/* 5 = strlen(/dev/) */

#define MAX_BLOCKS 5
#define PAGE_SIZE 4096

#define FORCE_SYNC 1
#define OPTIONAL_SYNC 2

typedef struct atomic_cnt {
	int cnt;
	pthread_spinlock_t lock;
} atomic_cnt;

struct lnvm_device {
	char dev[DISK_NAME_LEN];	/* open-channel SSD device */
	uint32_t dev_page_size;		/* Device page size */
	uint32_t write_page_size;	/* Write page size (depends on planes) */
	uint32_t nr_luns;		/* Number of LUNs exposed by the device*/
	uint32_t max_io_size;		/* Supported ppas in a single IO*/
	atomic_cnt ref_cnt;		/* Reference counter */
	UT_hash_handle hh;		/* hash handle for uthash */
};

struct lnvm_target {
	uint32_t version[3];
	uint32_t reserved;
	char tgtname[DISK_NAME_LEN];		/* dev to expose target as */
	char tgttype[NVM_TTYPE_NAME_MAX];	/* target type name */
	struct lnvm_device *dev;		/* Device associated */
	atomic_cnt ref_cnt;			/* Reference counter */
	UT_hash_handle hh;			/* hash handle for uthash */
};

struct lnvm_target_map {
	struct lnvm_target *tgt;
	int tgt_id;
	UT_hash_handle hh;			/* hash handle for uthash */
};

struct w_buffer {
	size_t cursize;		/* Current buf lenght. Follows mem */
	size_t cursync;		/* Bytes in buf that have been synced to media */
	size_t buf_limit;	/* Limit for the allocated memory region */
	void *buf;		/* Buffer to cache writes */
	char *mem;		/* Points to the place in buf where writes can be
				 * appended to. It defines the part of the
				 * buffer containing valid data */
	char *sync;		/* Points to the place in buf until which data
				 * has been synced to the media */
};

/* TODO: Allocate dynamic number of blocks */
struct beam {
	int gid;				/* internal global identifier */
	int lun;				/* virtual lun mapped to beam*/
	struct lnvm_target_map *tgt;		/* LightNVM target */
	struct vblock *current_w_vblock;	/* current block in use */
	struct vblock vblocks[MAX_BLOCKS];	/* vblocks forming the beam */
	int nvblocks;				/* number of vblocks */
	struct w_buffer w_buffer;		/* write buffer */
	unsigned long bytes;			/* valid bytes */
	UT_hash_handle hh;			/* hash handle for uthash */
};

static inline void atomic_init(struct atomic_cnt *cnt)
{
	pthread_spin_init(&cnt->lock, PTHREAD_PROCESS_SHARED);
}

static inline void atomic_set(struct atomic_cnt *cnt, int value)
{
	pthread_spin_lock(&cnt->lock);
	cnt->cnt = value;
	pthread_spin_unlock(&cnt->lock);
}

static inline void atomic_assign_inc(struct atomic_cnt *cnt, int *dst)
{
	pthread_spin_lock(&cnt->lock);
	cnt->cnt++;
	*dst = cnt->cnt;
	pthread_spin_unlock(&cnt->lock);
}

static inline void atomic_inc(struct atomic_cnt *cnt)
{
	pthread_spin_lock(&cnt->lock);
	cnt->cnt++;
	pthread_spin_unlock(&cnt->lock);
}

static inline int atomic_dec_and_test(struct atomic_cnt *cnt)
{
	int ret;

	pthread_spin_lock(&cnt->lock);
	cnt->cnt--;
	ret = (cnt->cnt == 0) ? 1 : 0;
	pthread_spin_unlock(&cnt->lock);

	return ret;
}

static inline size_t calculate_ppa_off(size_t cursync)
{
	size_t disaligned_data = cursync % PAGE_SIZE;
	size_t aligned_data = cursync / PAGE_SIZE;
	int rest = (disaligned_data == 0) ? 0 : 1;
	return (aligned_data + rest);
}

int flash_write(int tgt, struct vblock *vblock, const char *buf,
					size_t ppa_off, size_t count);
int flash_read(int tgt, struct vblock *vblock, void *buf, size_t ppa_off,
					size_t count);

#endif /* __FLASH_BEAM_H */
