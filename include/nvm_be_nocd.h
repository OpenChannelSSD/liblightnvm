/*
 * nvm_be_nocd - internal header
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
#ifndef __INTERNAL_NVM_BE_NOCD_H
#define __INTERNAL_NVM_BE_NOCD_H
#include <nvm_be_spdk.h>
#include <omp.h>

/**
 * Internal representation of NVM_BE_NOCD state
 */
struct nvm_be_nocd_state {
	struct nvm_be_spdk_state spdk;

	uint32_t ndescr;			///< #Chunk descriptors in rprt
	struct nvm_spec_rprt_descr descr[];	///< Chunk descriptors
};

void nvm_be_nocd_close(struct nvm_dev *dev);

struct nvm_dev *nvm_be_nocd_open(const char *dev_path, int flags);

struct nvm_async_ctx *nvm_be_nocd_async_init(struct nvm_dev *dev,
					     uint32_t depth, uint16_t flags);

int nvm_be_nocd_async_term(struct nvm_dev *dev, struct nvm_async_ctx *ctx);

int nvm_be_nocd_async_poke(struct nvm_dev *dev, struct nvm_async_ctx *ctx,
			    uint32_t max);

int nvm_be_nocd_async_wait(struct nvm_dev *dev, struct nvm_async_ctx *ctx);

struct nvm_spec_idfy *nvm_be_nocd_idfy(struct nvm_dev *dev,
				        struct nvm_ret *ret);

int nvm_be_nocd_gfeat(struct nvm_dev *dev, uint8_t id,
		       union nvm_nvme_feat *feat, struct nvm_ret *ret);

int nvm_be_nocd_sfeat(struct nvm_dev *dev, uint8_t id,
		       const union nvm_nvme_feat *feat, struct nvm_ret *ret);

struct nvm_spec_rprt *nvm_be_nocd_rprt(struct nvm_dev *dev,
					struct nvm_addr *addr, int opt,
					struct nvm_ret *ret);

struct nvm_spec_bbt *nvm_be_nocd_gbbt(struct nvm_dev *dev, struct nvm_addr addr,
				       struct nvm_ret *ret);

int nvm_be_nocd_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs, int naddrs,
		      uint16_t flags, struct nvm_ret *ret);

int nvm_be_nocd_scalar_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, uint16_t flags,
			      struct nvm_ret *ret);

int nvm_be_nocd_scalar_write(struct nvm_dev *dev, struct nvm_addr addr,
			     int naddrs, const void *data, const void *meta,
			     uint16_t flags, struct nvm_ret *ret);

int nvm_be_nocd_scalar_read(struct nvm_dev *dev, struct nvm_addr addr,
			     int naddrs, void *data, void *meta, uint16_t flags,
			     struct nvm_ret *ret);

int nvm_be_nocd_vector_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *meta, uint16_t flags,
			     struct nvm_ret *ret);

int nvm_be_nocd_vector_write(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, const void *data, const void *meta,
			     uint16_t flags, struct nvm_ret *ret);

int nvm_be_nocd_vector_read(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *data, void *meta, uint16_t flags,
			     struct nvm_ret *ret);

int nvm_be_nocd_vector_copy(struct nvm_dev *dev, struct nvm_addr src[],
			     struct nvm_addr dst[], int naddrs, uint16_t flags,
			     struct nvm_ret *ret);

#endif /* __INTERNAL_NVM_BE_NOCD */
