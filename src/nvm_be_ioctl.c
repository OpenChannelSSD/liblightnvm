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

	.open = nvm_be_nosys_open,
	.close = nvm_be_nosys_close,

	.idfy = nvm_be_nosys_idfy,
	.rprt = nvm_be_nosys_rprt,
	.sbbt = nvm_be_nosys_sbbt,
	.gbbt = nvm_be_nosys_gbbt,

	.erase = nvm_be_nosys_erase,
	.write = nvm_be_nosys_write,
	.read = nvm_be_nosys_read,
	.copy = nvm_be_nosys_copy,
};
#else
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
#include <nvm_be_ioctl.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

/*
static int ioctl_io(struct nvm_dev *dev, struct nvm_cmd *cmd,
		    struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_IOCTL_SUBMIT_IO, cmd);

	if (ret) {
		ret->result = 0x0;
		ret->status = 0x0;
	}

	if (err == -1)
		return err; // Propagate errno from IOCTL error

	return 0;
}

static int ioctl_am(struct nvm_dev *dev, struct nvm_cmd *cmd,
		    struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_IOCTL_ADMIN_CMD, cmd);

	if (ret) {
		ret->result = cmd->admin.result;
		ret->status = 0x0;
	}

	if (err == -1)
		return err;		// Propagate errno from IOCTL error

	if (cmd->vadmin.result) {	// Construct errno on cmd error
		errno = EIO;
		return -1;
	}

	return 0;
}
*/

static inline int ioctl_vio(struct nvm_dev *dev, struct nvm_cmd *cmd,
			    struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_SUBMIT_VIO, cmd);

	if (ret) {
		ret->result = cmd->vuser.result;
		ret->status = cmd->vuser.status;
	}

	if (err == -1)
		return err;		// Propagate errno from IOCTL error

	if (cmd->vuser.result) {	// Construct errno on cmd error
		errno = EIO;
		return -1;
	}

	return 0;
}

static inline int ioctl_vam(struct nvm_dev *dev, struct nvm_cmd *cmd,
			    struct nvm_ret *ret)
{
	const int err = ioctl(dev->fd, NVME_NVM_IOCTL_ADMIN_VIO, cmd);

	if (ret) {
		ret->result = cmd->vadmin.result;
		ret->status = cmd->vadmin.status;
	}

	if (err == -1)
		return err;		// Propagate errno from IOCTL error

	if (cmd->vadmin.result) {	// Construct errno on cmd error
		errno = EIO;
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

	cmd.vadmin.opcode = NVM_OPC_IDFY;
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

struct nvm_spec_rprt *nvm_be_ioctl_rprt(struct nvm_dev *dev,
					struct nvm_addr NVM_UNUSED(addr),
					uint16_t NVM_UNUSED(naddrs),
					int NVM_UNUSED(opts),
					struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	struct nvm_spec_rprt *rprt = NULL;
	const int rprt_len = 1474 * 8 + 8;
	int err;

	rprt = nvm_buf_alloca(0x1000, rprt_len);
	if (!rprt) {
		errno = ENOMEM;
		return NULL;
	}
	memset(rprt, 0, sizeof(char) * rprt_len);

	cmd.vadmin.opcode = NVM_S20_OPC_RPT; // Setup command
	cmd.vadmin.addr = (uint64_t)rprt;
	cmd.vadmin.data_len = rprt_len;
	cmd.vadmin.nppas = 0;

	err = ioctl_vam(dev, &cmd, ret);
	if (err) {
		NVM_DEBUG("FAILED: ioctl_vam");
		nvm_buf_free(rprt);
		return NULL; // NOTE: Propagate errno
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

	cmd.vadmin.opcode = NVM_S12_OPC_GET_BBT;
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
		if (nvm_addr_check(addrs[i], &dev->geo)) {
			NVM_DEBUG("FAILED: invalid addrs[i]");
			errno = EINVAL;
			return -1;
		}
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}

	cmd.vadmin.opcode = NVM_S12_OPC_SET_BBT; // Construct command
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

/**
 * Helper function for vector IO: erase/write/read
 */
static inline int cmd_ewr(struct nvm_dev *dev, struct nvm_addr addrs[],
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
	for (i = 0; i < naddrs; ++i)
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);

	// Unnatural numbers: counting from zero
	cmd.vuser.nppas = naddrs - 1;
	cmd.vuser.ppa_list = naddrs == 1 ? dev_addrs[0] : (uint64_t)dev_addrs;

	// Setup data
	cmd.vuser.addr = (uint64_t)data;
	cmd.vuser.data_len = data ? dev->geo.sector_nbytes * naddrs : 0;

	// Setup metadata
	cmd.vuser.metadata = (uint64_t)meta;
	cmd.vuser.metadata_len = meta ? dev->geo.meta_nbytes * naddrs : 0;

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

int nvm_be_ioctl_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  uint16_t flags, struct nvm_ret *ret)
{
	return cmd_ewr(dev, addrs, naddrs, NULL, NULL, flags, NVM_S12_OPC_ERASE,
		       ret);
}

int nvm_be_ioctl_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *data, void *meta, uint16_t flags,
		  struct nvm_ret *ret)
{
	return cmd_ewr(dev, addrs, naddrs, data, meta, flags, NVM_S12_OPC_WRITE,
		       ret);
}

int nvm_be_ioctl_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 void *data, void *meta, uint16_t flags,
		 struct nvm_ret *ret)
{
	return cmd_ewr(dev, addrs, naddrs, data, meta, flags, NVM_S12_OPC_READ,
		       ret);
}

int nvm_be_ioctl_copy(struct nvm_dev *dev, struct nvm_addr src[],
		      struct nvm_addr dst[], int naddrs,
		      uint16_t flags, struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	uint64_t src_dev[naddrs];
	uint64_t dst_dev[naddrs];
	int i, err;

	if (naddrs > NVM_NADDR_MAX) {
		errno = EINVAL;
		return -1;
	}

	cmd.vuser.opcode = NVM_S20_OPC_COPY;
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
	char nvme_name[NVM_DEV_PATH_LEN];
	int err;
	
	if (strlen(dev_path) > NVM_DEV_PATH_LEN) {
		NVM_DEBUG("FAILED: Device path too long\n");
		errno = EINVAL;
		return NULL;
	}

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		NVM_DEBUG("FAILED: allocating 'struct nvm_dev'\n");
		return NULL;	// Propagate errno from malloc
	}

	strncpy(dev->path, dev_path, NVM_DEV_PATH_LEN);
	strncpy(dev->name, dev_path+5, NVM_DEV_NAME_LEN);

	// HACK: use naming conventions to determine nsid, fallback to hardcode
	if (nvm_be_split_dpath(dev_path, nvme_name, &dev->nsid))
		dev->nsid = 1;

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
		return NULL;	// Propagate errno from open
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

	return dev;
}

void nvm_be_ioctl_close(struct nvm_dev *dev)
{
	close(dev->fd);
}

struct nvm_be nvm_be_ioctl = {
	.id = NVM_BE_IOCTL,

	.open = nvm_be_ioctl_open,
	.close = nvm_be_ioctl_close,

	.idfy = nvm_be_ioctl_idfy,
	.rprt = nvm_be_ioctl_rprt,
	.sbbt = nvm_be_ioctl_sbbt,
	.gbbt = nvm_be_ioctl_gbbt,

	.erase = nvm_be_ioctl_erase,
	.write = nvm_be_ioctl_write,
	.read = nvm_be_ioctl_read,
	.copy = nvm_be_ioctl_copy,
};
#endif
