/*
 * nvm_be_ioctl - internal header for IOCTL backend
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
#ifndef __INTERNAL_NVM_BE_IOCTL_H
#define __INTERNAL_NVM_BE_IOCTL_H

/**
 * Encapsulation of commonly used fields of (Open-Channel) NVMe commands
 */
struct nvm_cmd {
	union {
		struct {
			uint8_t opcode;		///< cdw00
			uint8_t flags;
			uint16_t control;
			uint16_t nppas;		///< cdw01
			uint16_t rsvd;
			uint64_t metadata;	///< cdw02-03
			uint64_t addr;		///< cdw04-05
			uint64_t ppa_list;	///< cdw06-07
			uint32_t metadata_len;	///< cdw08
			uint32_t data_len;	///< cdw09
			uint64_t status;	///< cdw10-11
			uint32_t result;	///< cdw12
			uint32_t rsvd3[3];	///< cdw13-15
		} vuser;	///< Common Open-Channel NVMe IO cmd. fields

		struct {
			uint8_t opcode;
			uint8_t flags;
			uint8_t rsvd[2];
			uint32_t nsid;
			uint32_t cdw2;
			uint32_t cdw3;
			uint64_t metadata;
			uint64_t addr;
			uint32_t metadata_len;
			uint32_t data_len;
			uint64_t ppa_list;
			uint16_t nppas;
			uint16_t control;
			uint32_t cdw13;
			uint32_t cdw14;
			uint32_t cdw15;
			uint64_t status;
			uint32_t result;
			uint32_t timeout_ms;
		} vadmin;	///< Common Open-Channel NVMe admin cmd. fields

		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t rsvd1;
			uint32_t nsid;
			uint32_t cdw2;
			uint32_t cdw3;
			uint64_t metadata;
			uint64_t addr;
			uint32_t metadata_len;
			uint32_t data_len;
			uint32_t cdw10;
			uint32_t cdw11;
			uint32_t cdw12;
			uint32_t cdw13;
			uint32_t cdw14;
			uint32_t cdw15;
			uint32_t timeout_ms;
			uint32_t result;
		} admin;	///< Common NVMe admin cmd. fields

		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t control;
			uint16_t nblocks;
			uint16_t rsvd;
			uint64_t metadata;
			uint64_t addr;
			uint64_t slba;
			uint32_t dsmgmt;
			uint32_t reftag;
			uint16_t apptag;
			uint16_t appmask;
		} user;		///< Common NVMe IO cmd. fields

		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t rsvd;
			uint32_t cdw[19];
		} shared;	///< Shared fields among commands

		uint32_t cdw[20];	///< Command as array of dwords
	};
};


/**
 * Execute an NVMe IO command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_be_ioctl_io(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret);

/**
 * Execute an NVMe admin command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_be_ioctl_am(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret);

/**
 * Execute an Open-Channel NVMe vector IO command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_be_ioctl_vio(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret);

/**
 * Execute an Open-Channel NVMe admin command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_vam(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret);

enum nvm_be_ioctl_flags {
	NVM_BE_IOCTL_WRITABLE = 0x1
};

struct nvm_dev *nvm_be_ioctl_open(const char *dev_path, int flags);

void nvm_be_ioctl_close(struct nvm_dev *dev);

struct nvm_spec_idfy *nvm_be_ioctl_idfy(struct nvm_dev *dev,
					struct nvm_ret *ret);

struct nvm_spec_rprt *nvm_be_ioctl_rprt(struct nvm_dev *dev,
					struct nvm_addr addr, uint16_t naddrs,
					int opts, struct nvm_ret *ret);

struct nvm_spec_bbt *nvm_be_ioctl_gbbt(struct nvm_dev *dev,
				       struct nvm_addr addr,
				       struct nvm_ret *ret);

int nvm_be_ioctl_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs, int naddrs,
		      uint16_t flags, struct nvm_ret *ret);

int nvm_be_ioctl_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       uint16_t flags, struct nvm_ret *ret);

int nvm_be_ioctl_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       void *data, void *meta, uint16_t flags,
		       struct nvm_ret *ret);

int nvm_be_ioctl_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		      void *data, void *meta, uint16_t flags,
		      struct nvm_ret *ret);

int nvm_be_ioctl_copy(struct nvm_dev *dev, struct nvm_addr src[],
		      struct nvm_addr dst[], int naddrs, uint16_t flags,
		      struct nvm_ret *ret);

/**
 * Prints a text-representation of the given command
 *
 * @param cmd The command to print
 */
void nvm_cmd_pr(struct nvm_cmd *cmd);

/**
 * Prints a textual presentation of the vuser par of the given command
 *
 * @param cmd The command to print
 */
void nvm_be_ioctl_vio_pr(struct nvm_cmd *cmd);

#endif /* __INTERNAL_NVM_BE_IOCTL */
