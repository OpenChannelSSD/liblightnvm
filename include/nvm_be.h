/*
 * nvm_be - internal header for library backends
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
#ifndef __INTERNAL_NVM_BE_H
#define __INTERNAL_NVM_BE_H

/**
 * Backend interface
 */
struct nvm_be {

	/**
	 * Backend identifier
	 */
	enum nvm_be_id id;

	/**
	 * Backend name
	 */
	char name[64];

	/**
	 * Open a device
	 */
	struct nvm_dev *(*open)(const char *, int flags);

	/**
	 * Close the given device
	 */
	void (*close)(struct nvm_dev *);

	/**
	 * Execute identify command
	 */
	struct nvm_spec_idfy *(*idfy)(struct nvm_dev *, struct nvm_ret *);

	/**
	 * Execute report chunk command
	 */
	struct nvm_spec_rprt *(*rprt)(struct nvm_dev *, struct nvm_addr *, int,
				      struct nvm_ret *);

	/**
	 * Execute get feature command
	 */
	int (*gfeat)(struct nvm_dev *, uint8_t, union nvm_spec_feat *,
		     struct nvm_ret *);

	/**
	 * Execute set feature command
	 */
	int (*sfeat)(struct nvm_dev *, uint8_t, const union nvm_spec_feat *,
		     struct nvm_ret *);

	/**
	 * Execute get bad-block-table command
	 */
	struct nvm_spec_bbt *(*gbbt)(struct nvm_dev *, struct nvm_addr,
				     struct nvm_ret *);

	/**
	 * Execute set bad-block-table command
	 */
	int (*sbbt)(struct nvm_dev *, struct nvm_addr *, int, uint16_t,
		    struct nvm_ret *);

	/**
	 * Execute erase command
	 */
	int (*scalar_erase)(struct nvm_dev *, struct nvm_addr [], int, uint16_t,
			    struct nvm_ret *);

	/**
	 * Execute write command
	 */
	int (*scalar_write)(struct nvm_dev *, struct nvm_addr, int,
			    const void *, const void *, uint16_t,
			    struct nvm_ret *);

	/**
	 * Execute read command
	 */
	int (*scalar_read)(struct nvm_dev *, struct nvm_addr, int,
			   void *, void *, uint16_t, struct nvm_ret *);

	/**
	 * Execute vector-erase command
	 */
	int (*vector_erase)(struct nvm_dev *, struct nvm_addr [], int, void *,
			    uint16_t, struct nvm_ret *);

	/**
	 * Execute vector-write command
	 */
	int (*vector_write)(struct nvm_dev *dev, struct nvm_addr *,
			    int, const void *, const void *, uint16_t,
			    struct nvm_ret *);

	/**
	 * Execute vector-read command
	 */
	int (*vector_read)(struct nvm_dev *, struct nvm_addr *, int,
			   void *, void *, uint16_t, struct nvm_ret *);

	/**
	 * Execute vector-copy command
	 */
	int (*vector_copy)(struct nvm_dev *, struct nvm_addr *,
			   struct nvm_addr *, int, uint16_t, struct nvm_ret *);

	/**
	 * Initialize an asynchronous command context with
	 */
	struct nvm_async_ctx *(*async_init)(struct nvm_dev *, uint32_t,
					    uint16_t);

	/**
	 * Terminate an asynchronous command context
	 */
	int (*async_term)(struct nvm_dev *, struct nvm_async_ctx *);

	/**
	 * Attempt to read all asynchronous events from a given context
	 */
	int (*async_poke)(struct nvm_dev *, struct nvm_async_ctx *, uint32_t);

	/**
	 * Wait for completion of all asynchronous events on a given context
	 */
	int (*async_wait)(struct nvm_dev *, struct nvm_async_ctx *);
};

/**
 * Helper functions for not supported / not implemented commands
 */

struct nvm_dev* nvm_be_nosys_open(const char *dev_path, int flags);

void nvm_be_nosys_close(struct nvm_dev *dev);

struct nvm_spec_idfy *nvm_be_nosys_idfy(struct nvm_dev *dev,
					struct nvm_ret *ret);

struct nvm_spec_rprt *nvm_be_nosys_rprt(struct nvm_dev *dev,
					struct nvm_addr *addr, int opt,
					struct nvm_ret *ret);

int nvm_be_nosys_gfeat(struct nvm_dev *, uint8_t, union nvm_spec_feat *,
		       struct nvm_ret *);

int nvm_be_nosys_sfeat(struct nvm_dev *, uint8_t, const union nvm_spec_feat *,
		       struct nvm_ret *);

struct nvm_spec_bbt *nvm_be_nosys_gbbt(struct nvm_dev *dev,
				       struct nvm_addr addr,
				       struct nvm_ret *ret);

int nvm_be_nosys_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs, int naddrs,
		      uint16_t flags, struct nvm_ret *ret);

int nvm_be_nosys_scalar_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, uint16_t flags, struct nvm_ret *ret);

int nvm_be_nosys_scalar_write(struct nvm_dev *dev, struct nvm_addr addr,
			      int naddrs, const void *data, const void *meta,
			      uint16_t flags, struct nvm_ret *ret);

int nvm_be_nosys_scalar_read(struct nvm_dev *dev, struct nvm_addr addr,
			     int naddrs, void *data, void *meta,
			     uint16_t flags, struct nvm_ret *ret);

int nvm_be_nosys_vector_erase(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, void *meta, uint16_t flags,
			      struct nvm_ret *ret);

int nvm_be_nosys_vector_write(struct nvm_dev *dev, struct nvm_addr addrs[],
			      int naddrs, const void *data, const void *meta,
			      uint16_t flags, struct nvm_ret *ret);

int nvm_be_nosys_vector_read(struct nvm_dev *dev, struct nvm_addr addrs[],
			     int naddrs, void *data, void *meta, uint16_t flags,
			     struct nvm_ret *ret);

int nvm_be_nosys_vector_copy(struct nvm_dev *dev, struct nvm_addr src[],
			     struct nvm_addr dst[], int naddrs, uint16_t flags,
			     struct nvm_ret *ret);

struct nvm_async_ctx *nvm_be_nosys_async_init(struct nvm_dev *dev,
					      uint32_t depth, uint16_t flags);

int nvm_be_nosys_async_term(struct nvm_dev *dev, struct nvm_async_ctx *ctx);

int nvm_be_nosys_async_poke(struct nvm_dev *dev, struct nvm_async_ctx *ctx,
			    uint32_t max);

int nvm_be_nosys_async_wait(struct nvm_dev *dev, struct nvm_async_ctx *ctx);

/**
 * Auxilary helpers
 */

/**
 * Splits a dev_path such as "/dev/nvme0n1" into {nvme_name: nvme0, nsid: 1}
 *
 * @returns 0 on success, 1 on error and errno set to indicate the error.
 */
int nvm_be_split_dpath(const char *dev_path, char *nvme_name, int *nsid);

/**
 * Fill out the geometry and other properties of the given device using the
 * idfy backend implementation
 */
int nvm_be_populate(struct nvm_dev *dev, struct nvm_be *be);

/**
 * Fill out attribute values which are derived from those populated by
 * `nvm_be_populate` and determine device quirks
 */
int nvm_be_populate_derived(struct nvm_dev *dev);

/**
 * Derives device quirks based on sysfs serial and device verid
 *
 * @param dev The device to determine quirks and assign quirk_mask
 * @return 0 on success, 1 on error and errno set to indicate the error
 */
int nvm_be_populate_quirks(struct nvm_dev *dev, const char serial[]);

extern struct nvm_be nvm_be_ioctl;
extern struct nvm_be nvm_be_lbd;
extern struct nvm_be nvm_be_spdk;

#endif /* __INTERNAL_NVM_BE_H */
