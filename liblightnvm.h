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

 * Written by Matias Bjorling <m@bjorling.me>
 *
 * liblightnvm Linux Open-Channel I/O interface
 */

#ifndef __LIBLIGHTNVM_H
#define __LIBLIGHTNVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include "ioctl.h"

/* mgmt.c */
int nvm_get_info(struct nvm_ioctl_info *);
int nvm_get_devices(struct nvm_ioctl_get_devices *);
int nvm_create_target(struct nvm_ioctl_create *);
int nvm_remove_target(struct nvm_ioctl_remove *);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
