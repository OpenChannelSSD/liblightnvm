/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <liblightnvm.h>

#include "ioctl.h"

int nvm_get_info(struct nvm_ioctl_info *u)
{
	return nvm_execute_ioctl(NVM_INFO, u);
}

int nvm_get_devices(struct nvm_ioctl_get_devices *u)
{
	return nvm_execute_ioctl(NVM_GET_DEVICES, u);
}

int nvm_create_target(struct nvm_ioctl_create *u)
{
	return nvm_execute_ioctl(NVM_DEV_CREATE, u);
}

int nvm_remove_target(struct nvm_ioctl_remove *u)
{
	return nvm_execute_ioctl(NVM_DEV_REMOVE, u);
}
