/*
 * be - Provides fall-back methods for backends which are not supported
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
#include <errno.h>
#include <stdlib.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

struct nvm_dev* nvm_be_nosys_open(const char *NVM_UNUSED(dev_path),
				  int NVM_UNUSED(flags))
{
	NVM_DEBUG("nvm_be_nosys_open");
	errno = ENOSYS;
	return NULL;
}

void nvm_be_nosys_close(struct nvm_dev *NVM_UNUSED(dev))
{
	NVM_DEBUG("nvm_be_nosys_close");
	errno = ENOSYS;
	return;
}

int nvm_be_nosys_user(struct nvm_dev *NVM_UNUSED(dev),
		      struct nvm_cmd *NVM_UNUSED(cmd),
		      struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_user");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_admin(struct nvm_dev *NVM_UNUSED(dev),
		       struct nvm_cmd *NVM_UNUSED(cmd),
		       struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_admin");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_vuser(struct nvm_dev *NVM_UNUSED(dev),
		       struct nvm_cmd *NVM_UNUSED(cmd),
		       struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_vuser");
	errno = ENOSYS;
	return -1;
}

int nvm_be_nosys_vadmin(struct nvm_dev *NVM_UNUSED(dev),
			struct nvm_cmd *NVM_UNUSED(cmd),
			struct nvm_ret *NVM_UNUSED(ret))
{
	NVM_DEBUG("nvm_be_nosys_vadmin");
	errno = ENOSYS;
	return -1;
}

