/*
 * cmd - Encapsulation of command construction and execution
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
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

int nvm_cmd_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  uint16_t flags, struct nvm_ret *ret)
{
	return dev->be->erase(dev, addrs, naddrs, flags, ret);
}

int nvm_cmd_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		  void *data, void *meta, uint16_t flags,
		  struct nvm_ret *ret)
{
	return dev->be->write(dev, addrs, naddrs, data, meta, flags, ret);
}

int nvm_cmd_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 void *data, void *meta, uint16_t flags,
		 struct nvm_ret *ret)
{
	return dev->be->read(dev, addrs, naddrs, data, meta, flags, ret);
}

int nvm_cmd_copy(struct nvm_dev *dev, struct nvm_addr src[],
		 struct nvm_addr dst[], int naddrs, uint16_t flags,
		 struct nvm_ret *ret)
{
	return dev->be->copy(dev, src, dst, naddrs, flags, ret);
}

struct nvm_spec_idfy *nvm_cmd_idfy(struct nvm_dev *dev, struct nvm_ret *ret)
{
	return dev->be->idfy(dev, ret);
}

struct nvm_spec_rprt *nvm_cmd_rprt(struct nvm_dev *dev, struct nvm_addr addr,
				   uint16_t naddrs, int opts,
				   struct nvm_ret *ret)
{
	return dev->be->rprt(dev, addr, naddrs, opts, ret);
}

struct nvm_spec_bbt *nvm_cmd_gbbt(struct nvm_dev *dev, struct nvm_addr addr,
				  struct nvm_ret *ret)
{
	return dev->be->gbbt(dev, addr, ret);
}

int nvm_cmd_sbbt(struct nvm_dev *dev, struct nvm_addr *addrs, int naddrs,
		 uint16_t flags, struct nvm_ret *ret)
{
	return dev->be->sbbt(dev, addrs, naddrs, flags, ret);
}

