/*
 * be_lba - Backend based on kernel support for NVM IOCTL and LBA for VECTOR IO
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_be_ioctl.h>
#include <nvm_be_sysfs.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

int nvm_be_lba_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd,
		     struct nvm_ret *ret)
{
	const size_t count = dev->geo.sector_nbytes;
	const uint8_t opcode = cmd->vuser.opcode;

	if (cmd->vuser.metadata || cmd->vuser.metadata_len) {
		errno = ENOSYS;
		return -1;
	}

	switch(opcode) {
	case NVM_S12_OPC_READ:
	case NVM_S12_OPC_WRITE:
		break;

	default:
		return nvm_be_ioctl_vuser(dev, cmd, ret);
	}

	for (uint16_t i = 0; i <= cmd->vuser.nppas; ++i) {
		uint64_t dev_addr, lba;
		ssize_t res;
		char *buf;
	
		if (cmd->vuser.nppas) {	// Get the addr on dev-format
			dev_addr = ((uint64_t *)cmd->vuser.ppa_list)[i];
		} else {
			dev_addr = cmd->vuser.ppa_list;
		}
					// Convert addr on dev-format to LBA/OFF
		lba = nvm_addr_dev2off(dev, dev_addr);

					// Get buffer offset
		buf = ((char *)cmd->vuser.addr) + (i * count);

		switch (opcode) {	// Perform the IO
		case NVM_S12_OPC_READ:
			res = pread(dev->fd, buf, count, lba);
			break;
		case NVM_S12_OPC_WRITE:
			res = pwrite(dev->fd, buf, count, lba);
			break;

		default:
			errno = ENOSYS;
			return -1;
		}

		if (res < 0)
			return -1;
	}

	return 0;
}

struct nvm_dev *nvm_be_lba_open(const char *dev_path, int NVM_UNUSED(flags))
{
	return nvm_be_ioctl_open(dev_path, NVM_BE_IOCTL_WRITABLE);
}

struct nvm_be nvm_be_lba = {
	.id = NVM_BE_LBA,

	.open = nvm_be_lba_open,
	.close = nvm_be_ioctl_close,

	.user = nvm_be_ioctl_user,
	.admin = nvm_be_ioctl_admin,

	.vuser = nvm_be_lba_vuser,
	.vadmin = nvm_be_ioctl_vadmin
};

