/*
 * flash_beam - I/O abstraction for flash memories.
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
	VBLOCK *current_w_vblock;	/* current block in use */
	VBLOCK vblocks[MAX_BLOCKS];	/* vblocks forming the beam */
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

static inline size_t calculate_ppa_off(size_t cursync, int write_page_size)
{
	size_t disaligned_data = cursync % write_page_size;
	size_t aligned_data = cursync / write_page_size;
	int rest = (disaligned_data == 0) ? 0 : 1;
	return (aligned_data + rest);
}

int flash_write(int tgt, VBLOCK *vblock, const char *buf,
				size_t ppa_off, size_t count,
				int max_pages_write, int write_page_size);
int flash_read(int tgt, VBLOCK *vblock, void *buf, size_t ppa_off,
				size_t count, int max_pages_read,
				int dev_page_size);

#endif /* __FLASH_BEAM_H */
