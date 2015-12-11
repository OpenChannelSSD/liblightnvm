/*
 * provisioning - LightNVM get/put block API
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
#include <liblightnvm.h>

#include "../util/debug.h"

int nvm_get_block(int tgt, uint32_t lun, NVM_VBLOCK *vblock)
{
	struct nvm_ioctl_provisioning prov = {
		.flags = 0,
	};
	struct nvm_ioctl_lun_status lun_status = {
		.nr_free_blocks = 0,
		.nr_inuse_blocks = 0,
		.nr_bad_blocks = 0,
	};
	int ret = 0;

#ifdef LNVM_DEBUG_ENABLED
	/* Initialize all bytes in structure, including padding for debugging */
	memset(&prov, 0, sizeof(prov));
#endif

	/* Initialize ioctl values */
	vblock->id = 0;
	vblock->bppa = 0;
	vblock->nppas = 0;
	vblock->ppa_bitmap = 0;

	vblock->vlun_id = lun;
	vblock->owner_id = 101;
	vblock->flags = 0x0;

	prov.vblock = vblock;
	prov.lun_status = &lun_status;
	prov.flags |= NVM_PROV_SPEC_LUN; //TODO: Expose in prov. API

	ret = ioctl(tgt, NVM_PR_GET_BLOCK, &prov);
	if (ret) {
		LNVM_DEBUG("Could not get block from lun %d\n", lun);
		goto out;
	}

	LNVM_DEBUG("Get block from lun %d. Block id:%llu bppa:%llu\n",
			vblock->vlun_id,
			vblock->id,
			vblock->bppa);
out:
	return ret;
}

int nvm_get_block_meta(int tgt, uint64_t vblock_id, NVM_VBLOCK *vblock)
{
	struct nvm_ioctl_provisioning prov = {
		.flags = 0,
	};
	struct nvm_ioctl_lun_status lun_status = {
		.nr_free_blocks = 0,
		.nr_inuse_blocks = 0,
		.nr_bad_blocks = 0,
	};
	int ret = 0;

#ifdef LNVM_DEBUG_ENABLED
	/* Initialize all bytes in structure, including padding for debugging */
	memset(&prov, 0, sizeof(prov));
#endif

	/* Initialize ioctl values */
	vblock->bppa = 0;
	vblock->nppas = 0;
	vblock->ppa_bitmap = 0;
	vblock->vlun_id = 0;
	vblock->owner_id = 0;
	vblock->flags = 0x0;

	vblock->id = vblock_id;

	prov.vblock = vblock;
	prov.lun_status = &lun_status;

	ret = ioctl(tgt, NVM_PR_GET_BLOCK_INFO, &prov);
	if (ret) {
		LNVM_DEBUG("Could not get metadata for block %lu\n", vblock_id);
		goto out;
	}

	LNVM_DEBUG("Get block medatada for block %llu. Lun: %d, bppa:%llu\n",
			vblock->id,
			vblock->vlun_id,
			vblock->bppa);
out:
	return ret;
}

int nvm_put_block(int tgt, NVM_VBLOCK *vblock)
{
	struct nvm_ioctl_provisioning prov = {
		.flags = 0,
	};
	struct nvm_ioctl_lun_status lun_status = {
		.nr_free_blocks = 0,
		.nr_inuse_blocks = 0,
		.nr_bad_blocks = 0,
	};
	int ret = 0;

#ifdef LNVM_DEBUG_ENABLED
	/* Initialize all bytes in structure, including padding for debugging */
	memset(&prov, 0, sizeof(prov));
#endif
	prov.vblock = vblock;
	prov.lun_status = &lun_status;

	ret = ioctl(tgt, NVM_PR_PUT_BLOCK, &prov);
	if (ret) {
		LNVM_DEBUG("Could not put block %llu (bppa:%llu) to lun %d\n",
				vblock->id, vblock->bppa, vblock->vlun_id);
		goto out;
	}

	LNVM_DEBUG("Put block %llu (bbpa:%llu) to lun %d\n",
			vblock->id, vblock->bppa, vblock->vlun_id);
out:
	return ret;
}

