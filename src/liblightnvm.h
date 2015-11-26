/* /usr/include/liblightnvm.h
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

 * Written by:
 *	- Javier Gonzalez <javier@cnexlabs.com>
 *	- Matias Bjorling <m@bjorling.me>
 *
 * liblightnvm Linux Open-Channel I/O interface
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
// int nvm_get_nluns();

/* Append store functionality - flash_append.c */
int nvm_target_open(const char *tgt, int flags);
void nvm_target_close(int tgt);
int nvm_beam_create(int tgt, int lun, int flags);
void nvm_beam_destroy(int beam, int flags);
ssize_t nvm_beam_append(int beam, const void *buf, size_t count);
ssize_t nvm_beam_read(int beam, void *buf, size_t count, off_t offset, int flags);
int nvm_beam_sync(int beam, int flags);

int nvm_init();
void nvm_exit();

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
