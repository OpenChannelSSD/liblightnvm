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
	 * Open a device
	 */
	struct nvm_dev *(*open)(const char *, int flags);

	/**
	 * Close the given device
	 */
	void (*close)(struct nvm_dev *);

	/**
	 * Send an user command to device
	 */
	int (*user)(struct nvm_dev *, struct nvm_cmd *, struct nvm_ret *);

	/**
	 * Send an admin command to device
	 */
	int (*admin)(struct nvm_dev *, struct nvm_cmd *, struct nvm_ret *);

	/**
	 * Send a vector user command to device
	 */
	int (*vuser)(struct nvm_dev *, struct nvm_cmd *, struct nvm_ret *);

	/**
	 * Send a vector admin command to device
	 */
	int (*vadmin)(struct nvm_dev *, struct nvm_cmd *, struct nvm_ret *);
};

struct nvm_dev* nvm_be_nosys_open(const char *dev_path, int flags);

void nvm_be_nosys_close(struct nvm_dev *dev);

int nvm_be_nosys_user(struct nvm_dev *dev, struct nvm_cmd *cmd,
		      struct nvm_ret *ret);

int nvm_be_nosys_admin(struct nvm_dev *dev, struct nvm_cmd *cmd,
		       struct nvm_ret *ret);

int nvm_be_nosys_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd,
		       struct nvm_ret *ret);

int nvm_be_nosys_vadmin(struct nvm_dev *dev, struct nvm_cmd *cmd,
			struct nvm_ret *ret);

/**
 * Splits a dev_path such as "/dev/nvme0n1" into {nvme_name: nvme0, nsid: 1}
 *
 * @returns 0 on success, 1 on error and errno set to indicate the error.
 */
int nvm_be_split_dpath(const char *dev_path, char *nvme_name, int *nsid);

/**
 * Fill out the geometry and other properties of the given device using the
 * given vadmin-backend function.
 */
int nvm_be_populate(struct nvm_dev *dev,
	int (*vadmin)(struct nvm_dev *, struct nvm_cmd *, struct nvm_ret *));

/**
 * Fill out attribute values which are derived from those populated by
 * `nvm_be_populate` and determine device quirks
 */
int nvm_be_populate_derived(struct nvm_dev *dev);

extern struct nvm_be nvm_be_ioctl;
extern struct nvm_be nvm_be_sysfs;
extern struct nvm_be nvm_be_lba;

#endif /* __INTERNAL_NVM_BE_H */
