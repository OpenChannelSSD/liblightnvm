/*
 * nvm_cmd - internal header for liblightnvm
 *
 * Copyright (C) Simon A. F. Lund <slund@cnexlabs.com>
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
#ifndef __INTERNAL_NVM_CMD_H
#define __INTERNAL_NVM_CMD_H

#include <liblightnvm.h>
#include <errno.h>

struct nvm_cmd_wrap {
	struct nvm_dev *dev;
	struct nvm_ret *ret;

	struct nvm_nvme_cmd cmd;

	void *meta;
	size_t meta_len;

	void *data;
	size_t data_len;

	struct nvm_nvme_dsm_range *dsmr_dma;
	size_t dsmr_len;

	int naddrs;
	uint64_t addr_dfmt;	// Address on device-format for SCALAR CMDS

	uint64_t *addrs_dma;	// DMA-allocated addresses
	size_t addrs_len;	// # nbytes of DMA-allocated addresses
	uint64_t *dst_dma;	// DMA-allocated destination addresses

	int completed;		// When used in SYNC callbacks
};

struct nvm_cmd_wrap *nvm_cmd_wrap_setup(struct nvm_dev *dev, int opcode,
					void *data, void *meta,
					struct nvm_addr addrs[],
					struct nvm_addr dst[],
					int naddrs,
					int flags,
					struct nvm_ret *ret);

struct nvm_cmd_wrap *nvm_cmd_wrap_pass(struct nvm_dev *dev,
				       struct nvm_nvme_cmd *cmd,
				       void *data, size_t data_nbytes,
				       void *meta, size_t meta_nbytes,
				       int flags, struct nvm_ret *ret);

void nvm_cmd_wrap_term(struct nvm_cmd_wrap *wrap);

void nvm_cmd_wrap_cpl(struct nvm_cmd_wrap *wrap,
		      const struct nvm_nvme_cpl *cpl);

#endif /* __INTERNAL_NVM_CMD_H */
