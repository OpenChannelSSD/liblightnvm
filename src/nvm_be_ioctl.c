/*
 * be_ioctl - Backend using for kernels with NVME_IOCTL_* & NVME_NVM_IOCTL_*
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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/lightnvm.h>
#include <linux/nvme_ioctl.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_debug.h>
#include <nvm_spec.h>

static inline uint64_t _ilog2(uint64_t x)
{
	uint64_t val = 0;

	while (x >>= 1)
		val++;

	return val;
}

int nvm_be_ioctl_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd,
		       struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, cmd);

	if (ret) {
		ret->result = cmd->vuser.result;
		ret->status = cmd->vuser.status;
	}

	if (err || cmd->vuser.result) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_be_ioctl_vadmin(struct nvm_dev *dev, struct nvm_cmd *cmd,
			struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_ADMIN_VIO, cmd);

	if (ret) {
		ret->result = cmd->vadmin.result;
		ret->status = cmd->vadmin.status;
	}

	if (err || cmd->vadmin.result) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_be_ioctl_user(struct nvm_dev *dev, struct nvm_cmd *cmd,
		      struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_IOCTL_SUBMIT_IO, cmd);
	if (ret) {
		ret->result = 0x0;
		ret->status = 0x0;
	}
	if (err) {
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_be_ioctl_admin(struct nvm_dev *dev, struct nvm_cmd *cmd,
		       struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, cmd);
	if (ret) {
		ret->result = cmd->admin.result;
		ret->status = 0x0;
	}
	if (err) {
		errno = EIO;
		return -1;
	}

	return 0;
}

static inline void _construct_ppaf_mask(struct spec_ppaf_nand *ppaf,
					struct spec_ppaf_nand_mask *mask)
{
	for (int i = 0 ; i < 12; ++i) {
		if ((i % 2)) {
			// i-1 = offset
			// i = width
			mask->a[i/2] = (((uint64_t)1<< ppaf->a[i])-1) << ppaf->a[i-1];
		}
	}
}

static inline int _ioctl_fill_geo(struct nvm_dev *dev, struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	struct spec_identify *idf;
	int err;

	err = posix_memalign((void **)&idf, 4096, sizeof(*idf));
	if (err) {
		errno = ENOMEM;
		return -1;
	}

	cmd.vadmin.opcode = S12_OPC_IDF;	// Setup command
	cmd.vadmin.addr = (uint64_t)idf;
	cmd.vadmin.data_len = sizeof(*idf);

	err = nvm_be_ioctl_vadmin(dev, &cmd, ret);
	if (err) {
		free(idf);
		return -1;			// NOTE: Propagate errno
	}

	switch (idf->s.verid) {
	case SPEC_VERID_12:
		dev->geo.page_nbytes = idf->s12.grp[0].fpg_sz;
		dev->geo.sector_nbytes = idf->s12.grp[0].csecs;
		dev->geo.meta_nbytes = idf->s12.grp[0].sos;

		dev->geo.nchannels = idf->s12.grp[0].num_ch;
		dev->geo.nluns = idf->s12.grp[0].num_lun;
		dev->geo.nplanes = idf->s12.grp[0].num_pln;
		dev->geo.nblocks = idf->s12.grp[0].num_blk;
		dev->geo.npages = idf->s12.grp[0].num_pg;

		dev->ppaf = idf->s12.ppaf;
		break;

	case SPEC_VERID_20:
		dev->geo.sector_nbytes = idf->s20.csecs;
		dev->geo.meta_nbytes = idf->s20.sos;
		dev->geo.page_nbytes = idf->s20.mw_min * dev->geo.sector_nbytes;

		dev->geo.nchannels = idf->s20.num_ch;
		dev->geo.nluns = idf->s20.num_lun;
		dev->geo.nplanes = idf->s20.mw_opt / idf->s20.mw_min;
		dev->geo.nblocks = idf->s20.num_chk;
		dev->geo.npages = ((idf->s20.clba * idf->s20.csecs) / dev->geo.page_nbytes) / dev->geo.nplanes;
		dev->geo.nsectors = dev->geo.page_nbytes / dev->geo.sector_nbytes;

		dev->ppaf = idf->s20.ppaf;

		break;

	default:
		NVM_DEBUG("Unsupported Version ID(%d)", idf->s.verid);
		errno = ENOSYS;
		free(idf);
		return -1;
	}

	dev->verid = idf->s.verid;
	_construct_ppaf_mask(&dev->ppaf, &dev->mask);

	// WARN: HOTFIX for reports of unrealisticly large OOB area
	if (dev->geo.meta_nbytes > 100) {
		dev->geo.meta_nbytes = 16;	// Naively hope this is right
	}
	dev->geo.nsectors = dev->geo.page_nbytes / dev->geo.sector_nbytes;

	/* Derive total number of bytes on device */
	dev->geo.tbytes = dev->geo.nchannels * dev->geo.nluns * \
			  dev->geo.nplanes * dev->geo.nblocks * \
			  dev->geo.npages * dev->geo.nsectors * \
			  dev->geo.sector_nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	dev->ssw = _ilog2(dev->geo.sector_nbytes);

	/* Derive a default plane mode */
	switch(dev->geo.nplanes) {
	case 4:
		dev->pmode = NVM_FLAG_PMODE_QUAD;
		break;
	case 2:
		dev->pmode = NVM_FLAG_PMODE_DUAL;
		break;
	case 1:
		dev->pmode = NVM_FLAG_PMODE_SNGL;
		break;
	default:
		errno = EINVAL;
		free(idf);
		return -1;
	}

	dev->erase_naddrs_max = NVM_NADDR_MAX;
	dev->write_naddrs_max = NVM_NADDR_MAX;
	dev->read_naddrs_max = NVM_NADDR_MAX;

	dev->meta_mode = NVM_META_MODE_NONE;

	free(idf);

	return 0;
}

struct nvm_dev *nvm_be_ioctl_open(const char *dev_path)
{
	struct nvm_dev *dev = NULL;
	struct nvm_ret ret = {0,0};
	int err;
	
	if (strlen(dev_path) > NVM_DEV_PATH_LEN) {
		NVM_DEBUG("FAILED: Device path too long\n");
		errno = EINVAL;
		return NULL;
	}

	dev = malloc(sizeof(*dev));
	if (!dev) {
		NVM_DEBUG("FAILED: allocating 'struct nvm_dev'\n");
		return NULL;	// Propagate errno from malloc
	}

	strncpy(dev->path, dev_path, NVM_DEV_PATH_LEN);
	strncpy(dev->name, dev_path+5, NVM_DEV_NAME_LEN);

	dev->fd = open(dev->path, O_RDWR | O_DIRECT);
	if (dev->fd < 0) {
		NVM_DEBUG("FAILED: open(dev->path(%s)), dev->fd(%d)\n",
			  dev->path, dev->fd);

		free(dev);

		return NULL;	// Propagate errno from open
	}

	err = _ioctl_fill_geo(dev, &ret);
	if (err) {
		NVM_DEBUG("FAILED: _ioctl_fill_geo, err(%d)", err);

		close(dev->fd);
		free(dev);

		return NULL;
	}

	return dev;
}

void nvm_be_ioctl_close(struct nvm_dev *dev)
{
	close(dev->fd);
}

struct nvm_be nvm_be_ioctl = {
	.open = nvm_be_ioctl_open,
	.close = nvm_be_ioctl_close,

	.user = nvm_be_ioctl_user,
	.admin = nvm_be_ioctl_admin,

	.vuser = nvm_be_ioctl_vuser,
	.vadmin = nvm_be_ioctl_vadmin,
};

