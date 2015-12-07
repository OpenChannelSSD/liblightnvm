/* /usr/include/liblightnvm.h
 *
 * liblightnvm - Linux Open-Channel I/O interface
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
#ifndef __LIBLIGHTNVM_H
#define __LIBLIGHTNVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>
#include <linux/lightnvm.h>

/* Management - mgmt.c */
int nvm_get_info(struct nvm_ioctl_info *);
int nvm_get_devices(struct nvm_ioctl_get_devices *);
int nvm_create_target(struct nvm_ioctl_tgt_create *);
int nvm_remove_target(struct nvm_ioctl_tgt_remove *);
int nvm_get_device_info(struct nvm_ioctl_dev_info *);
int nvm_get_target_info(struct nvm_ioctl_tgt_info *);

// int nvm_get_lun_info(int lun);

/* Append store functionality - flash_append.c */
int nvm_beam_create(const char *tgt, int lun, int flags);
void nvm_beam_destroy(int beam, int flags);
ssize_t nvm_beam_append(int beam, const void *buf, size_t count);
ssize_t nvm_beam_read(int beam, void *buf, size_t count, off_t offset,
								int flags);
int nvm_beam_sync(int beam, int flags);

int nvm_init();
void nvm_exit();

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
