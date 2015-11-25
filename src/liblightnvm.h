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

/* mgmt.c */
int nvm_get_info(struct nvm_ioctl_info *);
int nvm_get_devices(struct nvm_ioctl_get_devices *);
int nvm_create_target(struct nvm_ioctl_tgt_create *);
int nvm_remove_target(struct nvm_ioctl_tgt_remove *);
int nvm_get_device_info(struct nvm_ioctl_dev_info *);
int nvm_get_target_info(struct nvm_ioctl_tgt_info *);

/* core library */
int nvm_init();
void nvm_fini();
int nvm_get_nbeams();
int nvm_get_beam_prop(int beam_id);

/* flash_append.c */
int nvm_target_open(const char *tgt, int flags);
void nvm_target_close(int tgt);
int nvm_file_create(int tgt, int beam_id, int flags);
void nvm_file_delete(int fid, int flags);
int nvm_file_open(int fid, int flags);
void nvm_file_close(int fd, int flags);
ssize_t nvm_file_append(int fd, const void *buf, size_t count);
ssize_t nvm_file_read(int fd, void *buf, size_t count, off_t offset, int flags);
int nvm_file_sync(int fd, int flags);

/* unittests */
int nvm_test_lib();

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
