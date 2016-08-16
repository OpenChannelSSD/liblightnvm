/*
 * mgmt - management interface for Linux Open-Channel SSD (LightNVM)
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
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
#include <string.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>
#include <nvm_utils.h>

int nvm_mgmt_tgt_create(const char *tgt_name, const char *tgt_type_name,
                        const char *dev_name, int lun_begin, int lun_end)
{
	struct nvm_ioctl_create ctl;

	memset(&ctl, 0, sizeof(ctl));
	strncpy(ctl.dev, dev_name, DISK_NAME_LEN);
	strncpy(ctl.tgtname, tgt_name, DISK_NAME_LEN);
	strncpy(ctl.tgttype, tgt_type_name, NVM_TTYPE_NAME_MAX);
	ctl.conf.s.lun_begin = lun_begin;
	ctl.conf.s.lun_end = lun_end;

	return nvm_execute_ioctl(NVM_DEV_CREATE, &ctl);
}

int nvm_mgmt_tgt_remove(const char* tgt_name)
{
	struct nvm_ioctl_remove ctl;

	memset(&ctl, 0, sizeof(ctl));
	strncpy(ctl.tgtname, tgt_name, DISK_NAME_LEN);

	return nvm_execute_ioctl(NVM_DEV_REMOVE, &ctl);
}

