/*
 * provisioning - LightNVM get/put block API
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
#ifndef __LLNVM_PROVISION_H
#define __LLNVM_PROVISION_H

#include <stdint.h>

#include "ioctl.h"
#include "../util/uthash.h"

struct vblock {
	uint64_t id;
	uint64_t bppa;
	uint32_t beam_id;
	uint32_t owner_id;
	uint32_t nppas;
	uint16_t ppa_bitmap;
	uint16_t flags;
};

static inline int get_npages_block(struct vblock *vblock)
{
	return vblock->nppas;
}

/* provisioning */
int get_block(int tgt, uint32_t beam_id, struct vblock *vblock);
// int get_block(int tgt, uint32_t beam_id, void *meta, uint16_t meta_size,
							// struct vblock *vblock);
int get_block_meta(int tgt, uint64_t vblock_id, struct vblock *vblock);
int put_block(int tgt, struct vblock *vblock);

#endif

