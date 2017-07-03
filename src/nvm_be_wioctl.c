/*
 * be_wioctl - Backend using Windows DeviceIoControl
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
#ifndef NVM_BE_WIOCTL_ENABLED
#include <liblightnvm.h>
#include <nvm_be.h>
struct nvm_be nvm_be_wioctl = {
	.id = NVM_BE_WIOCTL,

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.user = nvm_be_nosys_user,
	.admin = nvm_be_nosys_admin,

	.vuser = nvm_be_nosys_vuser,
	.vadmin = nvm_be_nosys_vadmin
};
#else
#include <windows.h>
#include <winioctl.h>
#include <ntddscsi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <windows/ofa/nvme.h>
#include <windows/ofa/nvmeReg.h>
#include <windows/ofa/nvmeIoctl.h>
#include <windows/ofa/nvmeLnvm.h>
#include <liblightnvm.h>
#include <liblightnvm_spec.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_debug.h>

static inline int nvm_be_wioctl_command(struct nvm_dev *dev,
					struct nvm_cmd *cmd,
					struct nvm_ret *ret)
{
	BOOL bResult;
	DWORD dJunk;

	UCHAR buf[NVME_LNVM_IOCTL_NBYTES] = { 0 };

	PNVME_LNVM_IOCTL nIoctl = (PNVME_LNVM_IOCTL)buf;
	PNVMe_LNVM_COMMAND pLnvmCmd = (PNVMe_LNVM_COMMAND)nIoctl->NVMeCmd;

	int opcode = cmd->vuser.opcode;

	if (ret) {
		ret->result = 0x0;
		ret->status = 0x0;
	}

	NVM_DEBUG("sizeof(buf): %zu, NVME_LNVM_IOCTL_NBYTES: %zu\n",
		  sizeof(buf), NVME_LNVM_IOCTL_NBYTES);

	NVM_DEBUG("vadmin.data_len: %lu, vuser.data_len: %lu",
		  cmd->vadmin.data_len, cmd->vuser.data_len);

	// Set up the SRB IO Control header
	nIoctl->SrbIoCtrl.ControlCode = (ULONG)NVME_LNVM_SRB_IO_CODE;
	memcpy(nIoctl->SrbIoCtrl.Signature, NVME_SIG_STR, sizeof(NVME_SIG_STR));

	nIoctl->SrbIoCtrl.HeaderLength = (ULONG) sizeof(SRB_IO_CONTROL);
	nIoctl->SrbIoCtrl.Timeout = 30;
	nIoctl->SrbIoCtrl.Length = NVME_LNVM_IOCTL_NBYTES - sizeof(SRB_IO_CONTROL);

	pLnvmCmd->OPC = opcode;

	switch (opcode) {		// Setup the IOCTL
	case NVM_S12_OPC_GET_BBT:
	case NVM_S12_OPC_SET_BBT:
		errno = ENOSYS;
		return -1;

	case NVM_S12_OPC_IDF:
		nIoctl->DataLength = cmd->vadmin.data_len;
		break;

	case NVM_S12_OPC_WRITE:
		memcpy(nIoctl->Payload, (void*)cmd->vuser.addr,
		       cmd->vuser.data_len);

		if (cmd->vuser.metadata_len)
			memcpy(nIoctl->Payload + cmd->vuser.data_len,
			       (void*)cmd->vuser.metadata,
			       cmd->vuser.metadata_len);

	case NVM_S12_OPC_READ:
		nIoctl->DataLength = cmd->vuser.data_len;
		nIoctl->MetaLength = cmd->vuser.metadata_len;

	case NVM_S12_OPC_ERASE:
		nIoctl->NumAddrs = cmd->vuser.nppas;
		for (int idx = 0; idx <= cmd->vuser.nppas; ++idx)
			nIoctl->SrcAddrs[idx] = cmd->vuser.nppas ? ((uint64_t *)cmd->vuser.ppa_list)[idx] : cmd->vuser.ppa_list;

		pLnvmCmd->control = cmd->vuser.control;
		break;

	default:
		NVM_DEBUG("Invalid opcode: 0x%x", opcode);

		errno = EINVAL;
		return -1;
	}

	bResult = DeviceIoControl(dev->handle, IOCTL_SCSI_MINIPORT,
				  nIoctl, NVME_LNVM_IOCTL_NBYTES,
				  nIoctl, NVME_LNVM_IOCTL_NBYTES,
				  &dJunk, NULL);
	if (!bResult) {
		DWORD dError = GetLastError();

		NVM_DEBUG("wioctl-GetLastError: %lu, 0x%lx", dError, dError);

		errno = EIO;
		return -1;
	}

	switch (opcode) {
	case NVM_S12_OPC_IDF:
	case NVM_S12_OPC_GET_BBT:
		memcpy((void*)cmd->vadmin.addr, nIoctl->Payload,
		       cmd->vadmin.data_len);
		break;

	case NVM_S12_OPC_READ:
		memcpy((void*)cmd->vuser.addr, nIoctl->Payload,
		       cmd->vuser.data_len);
		if (cmd->vuser.metadata_len)
			memcpy((void*)cmd->vuser.metadata,
				nIoctl->Payload + cmd->vuser.data_len,
				cmd->vuser.metadata_len);
		break;

	case NVM_S12_OPC_SET_BBT:
	case NVM_S12_OPC_WRITE:
	case NVM_S12_OPC_ERASE:
		break;

	default:
		NVM_DEBUG("Invalid cmd->vuser.opcode: 0x%x", cmd->vuser.opcode);
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static inline uint64_t _ilog2(uint64_t x)
{
	uint64_t val = 0;

	while (x >>= 1)
		val++;

	return val;
}

static inline void _construct_ppaf_mask(struct nvm_spec_ppaf_nand *ppaf,
					struct nvm_spec_ppaf_nand_mask *mask)
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
	struct nvm_spec_identify *idf;
	int err;

	idf = nvm_buf_alloca(4096, sizeof(*idf));
	if (!idf) {
		errno = ENOMEM;
		return -1;
	}
	memset(idf, 0, sizeof(*idf));

	cmd.vadmin.opcode = NVM_S12_OPC_IDF;	// Setup command
	cmd.vadmin.addr = (uint64_t)idf;
	cmd.vadmin.data_len = sizeof(*idf);

	err = nvm_be_wioctl_command(dev, &cmd, ret);
	if (err) {
		nvm_buf_free(idf);
		return -1;			// NOTE: Propagate errno
	}

	switch (idf->s.verid) {
	case NVM_SPEC_VERID_12:
		dev->geo.page_nbytes = idf->s12.grp[0].fpg_sz;
		dev->geo.sector_nbytes = idf->s12.grp[0].csecs;
		dev->geo.meta_nbytes = idf->s12.grp[0].sos;

		dev->geo.nchannels = idf->s12.grp[0].num_ch;
		dev->geo.nluns = idf->s12.grp[0].num_lun;
		dev->geo.nplanes = idf->s12.grp[0].num_pln;
		dev->geo.nblocks = idf->s12.grp[0].num_blk;
		dev->geo.npages = idf->s12.grp[0].num_pg;

		dev->ppaf = idf->s12.ppaf;
		dev->mccap = idf->s12.grp[0].mccap;
		break;

	case NVM_SPEC_VERID_20:
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
		dev->mccap = idf->s20.mccap;
		break;

	default:
		NVM_DEBUG("Unsupported Version ID(%d)", idf->s.verid);
		errno = ENOSYS;
		nvm_buf_free(idf);
		return -1;
	}

	dev->verid = idf->s.verid;
	_construct_ppaf_mask(&dev->ppaf, &dev->mask);

	// WARN: HOTFIX for reports of unrealisticly large OOB area
	if (dev->geo.meta_nbytes > (dev->geo.sector_nbytes * 0.1)) {
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
		nvm_buf_free(idf);
		return -1;
	}

	// FIX: Lowering this from NVM_NADDR_MAX since Windows IOCTL will not
	// allow transferring more 256K 
	dev->erase_naddrs_max = 32;
	dev->write_naddrs_max = 32;
	dev->read_naddrs_max = 32;

	dev->meta_mode = NVM_META_MODE_NONE;

	nvm_buf_free(idf);

	return 0;
}

int nvm_be_wioctl_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd,
			struct nvm_ret *ret)
{
	return nvm_be_wioctl_command(dev, cmd, ret);
}

int nvm_be_wioctl_vadmin(struct nvm_dev *dev, struct nvm_cmd *cmd,
			 struct nvm_ret *ret)
{
	return nvm_be_wioctl_command(dev, cmd, ret);
}

struct nvm_dev *nvm_be_wioctl_open(const char *dev_path, int flags)
{
	struct nvm_dev *dev = NULL;
	struct nvm_ret ret = {0,0};
	int err;

	NVM_DEBUG("dev_path: %s, flags: %d\n", dev_path, flags);

	if ((strlen(dev_path) < 10) || (strlen(dev_path) > 60)) {
		NVM_DEBUG("FAILED: Invalid length of dev-path\n");
		errno = EINVAL;
		return NULL;
	}

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		NVM_DEBUG("FAILED: allocating 'struct nvm_dev'\n");
		return NULL;	// Propagate errno from malloc
	}

	strncpy(dev->path, dev_path, NVM_DEV_PATH_LEN);
	strncpy(dev->name, dev_path+4, NVM_DEV_NAME_LEN);

	// Setup handle to device file e.g. '\\.\SCSI1:'
	dev->handle = CreateFile(dev_path,
			  GENERIC_READ | GENERIC_WRITE,
			  FILE_SHARE_READ | FILE_SHARE_WRITE,
			  NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL);

	if (dev->handle == INVALID_HANDLE_VALUE) {
		free(dev);

		DWORD dw = GetLastError();

		switch(dw) {
		case 2:
			errno = ENOENT;
			return NULL;
		case 5:
			errno = EACCES;
			return NULL;
		case 123:
		case 161:
			errno = EINVAL;
			return NULL;
		default:
			NVM_DEBUG("Unhandled error-mapping dw: %lu", dw);

			errno = ENOSYS;
			return NULL;
		}
	}

	err = _ioctl_fill_geo(dev, &ret);
	if (err) {
		NVM_DEBUG("FAILED: _ioctl_fill_geo, err(%d)", err);

		CloseHandle(dev->handle);
		free(dev);

		return NULL;
	}

	return dev;
}

void nvm_be_wioctl_close(struct nvm_dev *dev)
{
	CloseHandle(dev->handle);
}

struct nvm_be nvm_be_wioctl = {
	.id = NVM_BE_WIOCTL,

	.open = nvm_be_wioctl_open,
	.close = nvm_be_wioctl_close,

	.user = nvm_be_nosys_user,
	.admin = nvm_be_nosys_admin,

	.vuser = nvm_be_wioctl_vuser,
	.vadmin = nvm_be_wioctl_vadmin,
};
#endif

