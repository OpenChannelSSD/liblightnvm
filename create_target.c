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

int nvm_create_target(struct nvm_ioctl_create *u)
{
	char dev[FILENAME_MAX] = NVM_CTRL_FILE;
	int ret, fd;

	fd = open(dev, O_WRONLY);
	if (fd < 0)
		return fd;

	memset(u, 0, sizeof(*u));

	ret = ioctl(fd, NVM_DEV_CREATE, u);
	if (ret)
		return ret;

	close(fd);
	return 0;
}
