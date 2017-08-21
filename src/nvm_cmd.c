/*
 * cmd - Lowest level command interface
 *
 * Copyright (C) 2015-2017 Javier Gonzáles <javier@cnexlabs.com>
 * Copyright (C) 2015-2017 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
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
#include <inttypes.h>
#include <stdio.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_utils.h>

int nvm_cmd_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	return dev->be->vuser(dev, cmd, ret);
}

int nvm_cmd_vadmin(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	return dev->be->vadmin(dev, cmd, ret);
}

int nvm_cmd_user(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	return dev->be->user(dev, cmd, ret);
}

int nvm_cmd_admin(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret)
{
	return dev->be->admin(dev, cmd, ret);
}

void nvm_cmd_vuser_pr(struct nvm_cmd *cmd)
{
	printf("cmd.vuser:\n");
	printf(" opcode: 0x%02"PRIx8"\n", cmd->vuser.opcode);
	printf(" flags: 0x%02"PRIx8"\n", cmd->vuser.flags);
	printf(" control: 0x%04"PRIx16"\n", cmd->vuser.control);
	printf(" nppas: %"PRIu16"\n", cmd->vuser.nppas);
	printf(" metadata: %"PRIu64"\n", cmd->vuser.metadata);
	printf(" addr: %"PRIu64"\n", cmd->vuser.addr);
	/*
	printf(" ppa_list {");
	for (int i = 0; i <= cmd->vuser.nppas; ++i) {
		printf("  0x%016lx,\n", ((uint64_t*)cmd->vuser.ppa_list)[i]);
	}*/
	printf(" metadata_len: %"PRIu32"\n", cmd->vuser.metadata_len);
	printf(" data_len: %"PRIu32"\n", cmd->vuser.data_len);
	printf(" status: 0x%016"PRIx64"\n", cmd->vuser.status);
	printf(" result: 0x%016"PRIx32"\n", cmd->vuser.result);
}

void nvm_cmd_pr(struct nvm_cmd *cmd)
{
	printf("nvm_cmd:\n");
	for (int32_t i = 0; i < 16; ++i)
		printf("  cdw%02"PRIi32": "NVM_I32_FMT"\n", i, NVM_I32_TO_STR(cmd->cdw[i]));
}

