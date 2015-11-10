/*
 * provision - LightNVM get/put block API
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
#include "ioctl.h"
#include "provisioning.h"
#include "../util/debug.h"

int get_block(int tgt, uint32_t vlun_id, struct vblock *vblock)
{
	int ret = 0;

	vblock->vlun_id = vlun_id;
	vblock->flags = 0x0;
	vblock->owner_id = 101;

	ret = ioctl(tgt, NVM_PR_GET_BLOCK, vblock);
	if (ret) {
		LNVM_DEBUG("Could not get block from lun %d\n", vlun_id);
		goto out;
	}

	LNVM_DEBUG("Get block from lun %d. Block id:%lu bppa:%lu\n",
			vblock->vlun_id,
			vblock->id,
			vblock->bppa);
out:
	return ret;
}

int get_block_meta(int tgt, uint64_t vblock_id, struct vblock *vblock)
{
	int ret = 0;

	vblock->id = vblock_id;

	ret = ioctl(tgt, NVM_PR_GET_BLOCK_META, vblock);
	if (ret) {
		LNVM_DEBUG("Could not get metadata for block %lu\n", vblock_id);
		goto out;
	}

	LNVM_DEBUG("Get block medatada for block %lu. Lun: %d, bppa:%lu\n",
			vblock->id,
			vblock->vlun_id,
			vblock->bppa);
out:
	return ret;
}

int put_block(int tgt, struct vblock *vblock)
{
	int ret = 0;

	ret = ioctl(tgt, NVM_PR_PUT_BLOCK, vblock);
	if (ret) {
		LNVM_DEBUG("Could not put block %lu to lun %d\n",
			vblock->id,
			vblock->vlun_id);
		goto out;
	}

	LNVM_DEBUG("Put block %lu to lun %d\n", vblock->id, vblock->vlun_id);
out:
	return ret;
}

/* TODO: Calculate it based on the stream the file is attached to. This should
 * be done on init, when creating a target, and should live in memory */
size_t get_npages_block(int tgt, uint32_t stream_id)
{
	int ret = 0;
	uint32_t npages = stream_id; /* TODO: Share a structure */

	ret = ioctl(tgt, NVM_DEV_NPAGES_BLOCK, &npages);
	if (ret) {
		LNVM_DEBUG("Could not get number of pages/block in stream %d\n",
				stream_id);
		goto out;
	}

	LNVM_DEBUG("Num. pages for blocks in stream %d: %d\n",
			stream_id, npages);
	ret = npages;
out:
	return ret;
}

