/*
 * be_ioctl - Backend sending commands via Linux kernel IOCTLs
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
#ifndef NVM_BE_IOCTL_ENABLED
#include <liblightnvm.h>
#include <nvm_be.h>
struct nvm_be nvm_be_ioctl = {
	.id = NVM_BE_IOCTL,
	.name = "NVM_BE_IOCTL",

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.idfy = nvm_be_nosys_idfy,
	.rprt = nvm_be_nosys_rprt,
	.gfeat = nvm_be_nosys_gfeat,
	.sfeat = nvm_be_nosys_sfeat,
	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.scalar_erase = nvm_be_nosys_scalar_erase,
	.scalar_write = nvm_be_nosys_scalar_write,
	.scalar_read = nvm_be_nosys_scalar_read,

	.vector_erase = nvm_be_nosys_vector_erase,
	.vector_write = nvm_be_nosys_vector_write,
	.vector_read = nvm_be_nosys_vector_read,
	.vector_copy = nvm_be_nosys_vector_copy,
};
#else
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/lightnvm.h>
#include <linux/nvme_ioctl.h>
#include <linux/fs.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_be_ioctl.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

static void *erase_meta_hack;

static inline int ioctl_vio(struct nvm_dev *dev, struct nvm_cmd *cmd,
			    struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, cmd);

	if (ret) {
		ret->result.vio.cs = cmd->vuser.status;
		ret->status = cmd->vuser.result;
	}

	if (err) {
		// Propagate errno from IOCTL error
		NVM_DEBUG("FAILED: err: %d", err);
		return -1;
	}

	if (cmd->vuser.result) {
		errno = EIO;
		NVM_DEBUG("FAILED: cmd->vuser.result: %d", cmd->vuser.result);
		return -1;
	}

	return 0;
}

static inline int ioctl_vam(struct nvm_dev *dev, struct nvm_cmd *cmd,
			    struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_ADMIN_VIO, cmd);

	if (ret) {
		ret->result.vio.cs = cmd->vadmin.status;
		ret->status = cmd->vadmin.result;
	}

	if (err) {
		// Propagate errno from IOCTL error
		NVM_DEBUG("FAILED: err: %d", err);
		return -1;
	}

	if (cmd->vadmin.result) {
		errno = EIO;
		NVM_DEBUG("FAILED: cmd->vadmin.result: %d", cmd->vadmin.result);
		return -1;
	}

	return 0;
}

void nvm_cmd_vio_pr(struct nvm_cmd *cmd)
{
	printf("cmd.vuser:\n");
	printf(" opcode: 0x%02"PRIx8"\n", cmd->vuser.opcode);
	printf(" flags: 0x%02"PRIx8"\n", cmd->vuser.flags);
	printf(" control: 0x%04"PRIx16"\n", cmd->vuser.control);
	printf(" nppas: %"PRIu16"\n", cmd->vuser.nppas);
	printf(" metadata: %"PRIu64"\n", cmd->vuser.metadata);
	printf(" addr: %"PRIu64"\n", cmd->vuser.addr);
	printf(" metadata_len: %"PRIu32"\n", cmd->vuser.metadata_len);
	printf(" data_len: %"PRIu32"\n", cmd->vuser.data_len);
	printf(" status: 0x%016"PRIx64"\n", cmd->vuser.status);
	printf(" result: 0x%016"PRIx32"\n", cmd->vuser.result);
}

void nvm_cmd_pr(struct nvm_cmd *cmd)
{
	printf("nvm_cmd:\n");
	for (int32_t i = 0; i < 16; ++i)
		printf("  cdw%02"PRIi32": "NVM_I32_FMT"\n", i,
		       NVM_I32_TO_STR(cmd->cdw[i]));
}

struct nvm_spec_idfy *nvm_be_ioctl_idfy(struct nvm_dev *dev,
					struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	struct nvm_spec_idfy *idfy = NULL;
	int err;

	idfy = nvm_buf_alloca(0x1000, sizeof(*idfy));
	if (!idfy) {
		errno = ENOMEM;
		return NULL;
	}
	memset(idfy, 0, sizeof(*idfy));

	cmd.vadmin.opcode = NVM_AOPC_IDFY;
	cmd.vadmin.addr = (uint64_t)idfy;
	cmd.vadmin.data_len = sizeof(*idfy);

	err = ioctl_vam(dev, &cmd, ret);
	if (err) {
		NVM_DEBUG("FAILED: ioctl_vam");
		nvm_buf_free(idfy);
		return NULL; // NOTE: Propagate errno
	}

	return idfy;
}

int nvm_be_ioctl_gfeat(struct nvm_dev *dev, uint8_t id,
		       union nvm_spec_feat *feat,
		       struct nvm_ret *NVM_UNUSED(ret))
{
	struct nvme_passthru_cmd cmd = { 0 };

	cmd.opcode = NVM_AOPC_GFEAT;
	cmd.nsid = dev->nsid;
	cmd.cdw10 = id;

	if(ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, &cmd)) {
		NVM_DEBUG("ioctl failed");
		return -1;
	}

	*((uint32_t *) feat) = cmd.result;

	return 0;
}

int nvm_be_ioctl_sfeat(struct nvm_dev *dev, uint8_t id,
		       const union nvm_spec_feat *feat,
		       struct nvm_ret *NVM_UNUSED(ret))
{
	struct nvme_passthru_cmd cmd = { 0 };

	cmd.opcode = NVM_AOPC_SFEAT;
	cmd.nsid = dev->nsid;
	cmd.cdw10 = id;
	cmd.cdw11 = *((uint32_t *) feat);

	if(ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, &cmd)) {
		NVM_DEBUG("ioctl failed");
		return -1;
	}

	return 0;
}

struct nvm_spec_rprt *nvm_be_ioctl_rprt(struct nvm_dev *dev,
					struct nvm_addr *addr,
					int NVM_UNUSED(opt),
					struct nvm_ret *NVM_UNUSED(ret))
{
	if (NVM_SPEC_VERID_20 != dev->verid) {
		errno = EINVAL;
		return NULL;
	}

	struct nvm_spec_rprt *rprt;

	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	const size_t lpo_off = addr ? nvm_addr_gen2lpo(dev, *addr) : 0;

	size_t ndescr;
	if (addr) {
		ndescr = geo->l.nchunk;
	} else {
		ndescr = geo->l.nchunk * geo->l.npunit * geo->l.npugrp;
	}

	const size_t descr_len = sizeof(struct nvm_spec_rprt_descr);
	const size_t rprt_len = ndescr * descr_len + sizeof(rprt->ndescr);

	rprt = nvm_buf_alloca(0x1000, rprt_len);
	if (!rprt) {
		errno = ENOMEM;
		return NULL;
	}
	memset(rprt, 0, rprt_len);

	rprt->ndescr = ndescr;

	const unsigned int max_descr_pr_call = 0x1000 * 4 / descr_len;
	size_t data_len;

	for (unsigned int i = 0; i < ndescr; i += max_descr_pr_call) {
		data_len = NVM_MIN(max_descr_pr_call, ndescr - i) * descr_len;

		struct nvme_passthru_cmd cmd = { 0 };

		cmd.opcode = NVM_AOPC_RPRT;
		cmd.nsid = dev->nsid;
		cmd.addr = (uint64_t) (uintptr_t) &rprt->descr[i];
		cmd.data_len = data_len;

		uint32_t numd = (data_len >> 2) - 1;
		uint16_t numdu = numd >> 16;
		uint16_t numdl = numd & 0xffff;

		uint64_t lpo = lpo_off + i * descr_len;

		cmd.cdw10 = 0xCA | (numdl << 16);
		cmd.cdw11 = numdu;
		cmd.cdw12 = lpo;
		cmd.cdw13 = (lpo >> 32);

		if(ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, &cmd)) {
			NVM_DEBUG("ioctl failed");
			free(rprt);
			return NULL;
		}
	}

	return rprt;
}

struct nvm_spec_bbt *nvm_be_ioctl_gbbt(struct nvm_dev *dev,
				       struct nvm_addr addr,
				       struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	struct nvm_spec_bbt *spec_bbt;
	size_t spec_bbt_sz;
	int err;

	uint32_t nblks = dev->geo.nblocks * dev->geo.nplanes;
	spec_bbt_sz = sizeof(*spec_bbt) + sizeof(*(spec_bbt->blk)) * nblks;
	spec_bbt = nvm_buf_alloc(&dev->geo, spec_bbt_sz);
	if (!spec_bbt) {
		NVM_DEBUG("FAILED: malloc k_bbt failed");
		errno = ENOMEM;
		return NULL;
	}

	cmd.vadmin.opcode = NVM_AOPC_GBBT;
	cmd.vadmin.addr = (uint64_t)spec_bbt;
	cmd.vadmin.data_len = spec_bbt_sz;
	cmd.vadmin.ppa_list = nvm_addr_gen2dev(dev, addr);
	cmd.vadmin.nppas = 0;

	err = ioctl_vam(dev, &cmd, ret);
	if (err || (spec_bbt->tblks != nblks)) {
		NVM_DEBUG("FAILED: be execution failed");
		errno = EIO;
		nvm_buf_free(spec_bbt);
		return NULL;
	}
	if (!(spec_bbt->tblid[0] == 'B' && spec_bbt->tblid[1] == 'B' &&
	      spec_bbt->tblid[2] == 'L' && spec_bbt->tblid[3] == 'T')) {
		NVM_DEBUG("FAILED: invalid format of returned bbt");
		errno = EIO;
		nvm_buf_free(spec_bbt);
		return NULL;
	}

	return spec_bbt;
}

int nvm_be_ioctl_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs, int naddrs,
		      uint16_t flags, struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	uint64_t dev_addrs[naddrs];
	int err;

	switch(flags) {
	case NVM_BBT_FREE:
	case NVM_BBT_BAD:
	case NVM_BBT_DMRK:
	case NVM_BBT_GBAD:
	case NVM_BBT_HMRK:
		break;
	default:
		NVM_DEBUG("FAILED: invalid mark");
		errno = EINVAL;
		return -1;
	}

	if (naddrs > NVM_NADDR_MAX) {
		NVM_DEBUG("FAILED: naddrs > NVM_NADDR_MAX");
		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < naddrs; ++i) { // Setup PPAs: Convert format
		if (nvm_addr_check(addrs[i], dev)) {
			NVM_DEBUG("FAILED: invalid addrs[i]");
			errno = EINVAL;
			return -1;
		}
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}

	cmd.vadmin.opcode = NVM_AOPC_SBBT; // Construct command
	cmd.vadmin.control = flags;
	cmd.vadmin.nppas = naddrs - 1; // Unnatural numbers: counting from zero
	cmd.vadmin.ppa_list = naddrs == 1 ? dev_addrs[0] : (uint64_t)dev_addrs;

	err = ioctl_vam(dev, &cmd, ret);
	if (err) {
		NVM_DEBUG("FAILED: be execution failed");
		errno = EIO;
		return -1;
	}

	return 0;
}

int nvm_be_ioctl_scalar_erase(struct nvm_dev *dev, struct nvm_addr *addrs,
			      int naddrs, uint16_t NVM_UNUSED(flags),
			      struct nvm_ret *NVM_UNUSED(ret))
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	struct nvm_nvme_dsm_range *dsmr = NULL;
	size_t dsmr_len = sizeof(*dsmr) * naddrs;
	struct nvme_passthru_cmd cmd = { 0 };
	int err = 0;

	if ((!naddrs) || (naddrs > NVM_NADDR_MAX)) {
		NVM_DEBUG("FAILED: invalid naddrs: %d", naddrs);
		errno = EINVAL;
		return -1;
	}

	dsmr = nvm_buf_alloc(geo, dsmr_len);
	if (!dsmr) {
		NVM_DEBUG("FAILED: nvm_buf_alloc of DSM range");
		return -1;
	}

	for(int idx = 0; idx < naddrs; ++idx) {
		dsmr[idx].cattr = 0;
		dsmr[idx].nlb = geo->l.nsectr;
		dsmr[idx].slba = nvm_addr_gen2dev(dev, addrs[idx]);
	}

	cmd.opcode = NVM_DOPC_SCALAR_ERASE;
	cmd.nsid = dev->nsid;
	cmd.addr = (__u64)(uintptr_t) dsmr;
	cmd.data_len = naddrs * sizeof(*dsmr);
	cmd.cdw10 = naddrs - 1;
	cmd.cdw11 = 0x1 << 2; // Assign Bit: Attribute Deallocate (AD)

	err = ioctl(dev->fd, NVME_IOCTL_IO_CMD, &cmd);
	if (err) {
		NVM_DEBUG("FAILED: ioctl-scalar failed");
		nvm_buf_free(dsmr);
		return -1;
	}

	nvm_buf_free(dsmr);

	return 0;
}

/**
 * Kernel-issue workaround, using deprecated IOCTL for scalar commands with meta
 * due to a bug in the kernel (see kernel commit 9b382768), we have to
 * use the deprecated ioctl NVME_IOCTL_SUBMIT_IO command to submit the
 * request if it uses meta data.
 *
 * NOTE: NVME_IOCTL_SUBMIT_IO does NOT work if the request does NOT have
 * metadata the bug is fixed in linux v4.18.
 */
static inline int cmd_scalar_wr_dep_ioc(struct nvm_dev *dev,
					struct nvm_addr addr, int naddrs,
					void *data, void *meta,
					uint16_t NVM_UNUSED(flags),
					uint16_t opcode,
					struct nvm_ret *NVM_UNUSED(ret))
{
	struct nvme_user_io cmd = { 0 };
	cmd.opcode = opcode;
	cmd.nblocks = naddrs - 1;
	cmd.metadata = (__u64)(uintptr_t) meta;
	cmd.addr = (__u64)(uintptr_t) data;
	cmd.slba = nvm_addr_gen2dev(dev, addr);

	int err;

	err = ioctl(dev->fd, NVME_IOCTL_SUBMIT_IO, &cmd);
	if (err) {
		NVM_DEBUG("FAILED: ioctl-scalar failed: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/**
 * Helper function for SCALAR IO: write/read
 */
static inline int cmd_scalar_wr(struct nvm_dev *dev, struct nvm_addr addr,
				int naddrs, void *data, void *meta,
				uint16_t flags, uint16_t opcode,
				struct nvm_ret *ret)
{
	if (meta) {
		return cmd_scalar_wr_dep_ioc(dev, addr, naddrs, data, meta,
					     flags, opcode, ret);
	}

	struct nvme_passthru_cmd cmd = { 0 };

	cmd.opcode = opcode;
	cmd.nsid = dev->nsid;
	cmd.metadata = (__u64)(uintptr_t) meta;
	cmd.addr = (__u64)(uintptr_t) data;
	cmd.metadata_len = meta ? dev->geo.meta_nbytes * naddrs : 0;
	cmd.data_len = dev->geo.sector_nbytes * naddrs;

	uint64_t slba = nvm_addr_gen2dev(dev, *addrs);

	cmd.cdw10 = slba;
	cmd.cdw11 = slba >> 32;
	cmd.cdw12 = naddrs - 1;

	int err;

	err = ioctl(dev->fd, NVME_IOCTL_IO_CMD, &cmd);
	if (err) {
		NVM_DEBUG("FAILED: ioctl-scalar failed: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int nvm_be_ioctl_scalar_write(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, void *data, void *meta,
			      uint16_t flags, struct nvm_ret *ret)
{
	return cmd_scalar_wr(dev, addrs, naddrs, data, meta, flags,
			     NVM_DOPC_SCALAR_WRITE, ret);
}

int nvm_be_ioctl_scalar_read(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *data, void *meta, uint16_t flags,
			     struct nvm_ret *ret)
{
	return cmd_scalar_wr(dev, addrs, naddrs, data, meta, flags,
			     NVM_DOPC_SCALAR_READ, ret);
}

/**
 * Helper function for vector IO: erase/write/read
 */
static inline int cmd_vector_ewr(struct nvm_dev *dev, struct nvm_addr addrs[],
			  int naddrs, void *data, void *meta,
			  uint16_t flags, uint16_t opcode,
			  struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	uint64_t dev_addrs[naddrs];
	int i, err;

	if (naddrs > NVM_NADDR_MAX) {
		errno = EINVAL;
		return -1;
	}

	cmd.vuser.opcode = opcode;
	cmd.vuser.control = flags | NVM_FLAG_DEFAULT;

	// Setup PPAs: Convert address format from generic to device specific
	for (i = 0; i < naddrs; ++i) {
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}

	// Unnatural numbers: counting from zero
	cmd.vuser.nppas = naddrs - 1;
	cmd.vuser.ppa_list = naddrs == 1 ? dev_addrs[0] : (uint64_t)dev_addrs;

	// Setup data
	cmd.vuser.addr = (uint64_t)data;
	cmd.vuser.data_len = data ? dev->geo.sector_nbytes * naddrs : 0;

	// Setup metadata
	cmd.vuser.metadata = (uint64_t)meta;
	cmd.vuser.metadata_len = meta ? dev->geo.meta_nbytes * naddrs : 0;

	if ((opcode == NVM_DOPC_VECTOR_ERASE) && meta) {
		// Fake data setup to force IOCTL kernel-handling to transfer meta
		cmd.vuser.addr = (uint64_t)erase_meta_hack;
		cmd.vuser.data_len = dev->geo.l.nbytes;

		cmd.vuser.metadata_len = sizeof(struct nvm_spec_rprt_descr) * naddrs;
	}

	err = ioctl_vio(dev, &cmd, ret);
	if (!err)
		return 0;		// No errors, we can return

	switch (cmd.vuser.result) {
	case 0x700:			// Ignore: Acceptable error
	case 0x4700:			// Ignore: Acceptable error
		return 0;

	default:
		return -1;		// Propagate errno from backend
	}
}

int nvm_be_ioctl_vector_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, void *meta, uint16_t flags,
			      struct nvm_ret *ret)
{
	return cmd_vector_ewr(dev, addrs, naddrs, NULL, meta, flags,
			      NVM_DOPC_VECTOR_ERASE, ret);
}

int nvm_be_ioctl_vector_write(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, void *data, void *meta,
			      uint16_t flags, struct nvm_ret *ret)
{
	return cmd_vector_ewr(dev, addrs, naddrs, data, meta, flags,
			      NVM_DOPC_VECTOR_WRITE, ret);
}

int nvm_be_ioctl_vector_read(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *data, void *meta, uint16_t flags,
			     struct nvm_ret *ret)
{
	return cmd_vector_ewr(dev, addrs, naddrs, data, meta, flags,
			      NVM_DOPC_VECTOR_READ, ret);
}

int nvm_be_ioctl_vector_copy(struct nvm_dev *dev, struct nvm_addr src[],
			     struct nvm_addr dst[], int naddrs, uint16_t flags,
			     struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	uint64_t src_dev[naddrs];
	uint64_t dst_dev[naddrs];
	int i, err;

	if (naddrs > NVM_NADDR_MAX) {
		errno = EINVAL;
		return -1;
	}

	cmd.vuser.opcode = NVM_DOPC_VECTOR_COPY;
	cmd.vuser.control = flags | NVM_FLAG_DEFAULT;

	// Setup PPAs: Convert address format from generic to device specific
	for (i = 0; i < naddrs; ++i) {
		src_dev[i] = nvm_addr_gen2dev(dev, src[i]);
		dst_dev[i] = nvm_addr_gen2dev(dev, dst[i]);
	}

	// Unnatural numbers: counting from zero
	cmd.vuser.nppas = naddrs - 1;
	cmd.vuser.ppa_list = naddrs == 1 ? src_dev[0] : (uint64_t)src_dev;

	// TODO: Set the dst addrs correctly
	cmd.vuser.rsvd3[0] = dst_dev[i];

	err = ioctl_vio(dev, &cmd, ret);
	if (!err)
		return 0;		// No errors, we can return

	switch (cmd.vuser.result) {
	case 0x700:			// Ignore: Acceptable error
	case 0x4700:			// Ignore: Acceptable error
		return 0;

	default:
		return -1;		// Propagate errno from backend
	}

	return 0;
}

struct nvm_dev *nvm_be_ioctl_open(const char *dev_path, int flags)
{
	struct nvm_dev *dev = NULL;
	struct stat dev_stat;
	int err;

	if (strlen(dev_path) > NVM_DEV_PATH_LEN) {
		NVM_DEBUG("FAILED: Device path too long\n");
		errno = EINVAL;
		return NULL;
	}

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		NVM_DEBUG("FAILED: allocating 'struct nvm_dev'\n");
		// Propagate errno from malloc
		return NULL;
	}

	strncpy(dev->path, dev_path, NVM_DEV_PATH_LEN);
	strncpy(dev->name, dev_path+5, NVM_DEV_NAME_LEN);

	switch (flags) {
	case NVM_BE_IOCTL_WRITABLE:
		dev->fd = open(dev->path, O_RDWR | O_DIRECT);
		break;

	default:
		dev->fd = open(dev->path, O_RDONLY);
		break;
	}
	if (dev->fd < 0) {
		NVM_DEBUG("FAILED: open(dev->path(%s)), dev->fd(%d)\n",
			  dev->path, dev->fd);
		free(dev);
		// Propagate errno from open
		return NULL;
	}

	err = fstat(dev->fd, &dev_stat);
	if (err < 0) {
		// Propagate errno from fstat
		return NULL;
	}

	if (!S_ISBLK(dev_stat.st_mode)) {
		NVM_DEBUG("FAILED: device is not a block device");
		errno = ENOTBLK;
		return NULL;
	}

	dev->nsid = ioctl(dev->fd, NVME_IOCTL_ID);
	if (dev->nsid < 1) {
		NVM_DEBUG("FAILED: failed retrieving nsid");
		// Propagate errno from ioctl
		return NULL;
	}

	struct nvme_admin_cmd cmd = { 0 };

	cmd.opcode = 0x06; // identify
	cmd.nsid = dev->nsid;
	cmd.addr = (uint64_t)(uintptr_t) &dev->ns;
	cmd.data_len = 0x1000;

	if(ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, &cmd)) {
		NVM_DEBUG("ioctl failed");
		return NULL;
	}

	err = nvm_be_populate(dev, &nvm_be_ioctl);
	if (err) {
		NVM_DEBUG("FAILED: nvm_be_populate");
		close(dev->fd);
		free(dev);
		return NULL;
	}

	err = nvm_be_populate_derived(dev);
	if (err) {
		NVM_DEBUG("FAILED: nvm_be_populate_derived");
		close(dev->fd);
		free(dev);
		return NULL;
	}

	erase_meta_hack = nvm_buf_alloc(&dev->geo, dev->geo.l.nbytes);
	if (!erase_meta_hack) {
		NVM_DEBUG("FAILED: erase_meta_hack alloc");
		close(dev->fd);
		free(dev);
		return NULL;
	}

	return dev;
}

void nvm_be_ioctl_close(struct nvm_dev *dev)
{
	nvm_buf_free(erase_meta_hack);
	close(dev->fd);
}

struct nvm_be nvm_be_ioctl = {
	.id = NVM_BE_IOCTL,
	.name = "NVM_BE_IOCTL",

	.open = nvm_be_ioctl_open,
	.close = nvm_be_ioctl_close,

	.idfy = nvm_be_ioctl_idfy,
	.rprt = nvm_be_ioctl_rprt,
	.gfeat = nvm_be_ioctl_gfeat,
	.sfeat = nvm_be_ioctl_sfeat,
	.sbbt = nvm_be_ioctl_sbbt,
	.gbbt = nvm_be_ioctl_gbbt,

	.scalar_erase = nvm_be_ioctl_scalar_erase,
	.scalar_write = nvm_be_ioctl_scalar_write,
	.scalar_read = nvm_be_ioctl_scalar_read,

	.vector_erase = nvm_be_ioctl_vector_erase,
	.vector_write = nvm_be_ioctl_vector_write,
	.vector_read = nvm_be_ioctl_vector_read,
	.vector_copy = nvm_be_ioctl_vector_copy,
};
#endif
