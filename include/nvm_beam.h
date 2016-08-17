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
#ifndef __NVM_BEAM_H
#define __NVM_BEAM_H

#include <stdio.h>
#include <stdlib.h>
#include <uthash.h>

#include <liblightnvm.h>
#include <nvm.h>

#define MAX_BLOCKS 5

#define FORCE_SYNC 1
#define OPTIONAL_SYNC 2

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
	struct nvm_tgt *tgt;			/* LightNVM target */
	struct nvm_vblock *current_w_vblock;	/* current block in use */
	struct nvm_vblock vblocks[MAX_BLOCKS];	/* vblocks forming the beam */
	int nvblocks;				/* number of vblocks */
	struct w_buffer w_buffer;		/* write buffer */
	unsigned long bytes;			/* valid bytes */
	UT_hash_handle hh;			/* hash handle for uthash */
};

static inline size_t calculate_ppa_off(size_t cursync, int write_page_size)
{
	size_t disaligned_data = cursync % write_page_size;
	size_t aligned_data = cursync / write_page_size;
	int rest = (disaligned_data == 0) ? 0 : 1;
	return (aligned_data + rest);
}

#endif /* __NVM_BEAM_H */
