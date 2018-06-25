/*
 * nvm_spec - internal header for liblightnvm
 *
 * Copyright (C) 2015-2017 Javier Gonzáles <javier@cnexlabs.com>
 * Copyright (C) 2015-2017 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
 * Copyright (C) 2018      Klaus Birkelund Jensen <klaus.jensen@cnexlabs.com>
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
#ifndef __INTERNAL_NVM_NVME_H
#define __INTERNAL_NVM_NVME_H

#include <stdint.h>

struct nvm_nvme_lbaf {
	// metadata size
	uint16_t ms;

	// lba data size
	uint8_t  ds;

	// relative performance
	uint8_t  rp;
};

struct nvm_nvme_ns {
	// namespace size
	uint64_t nsze;

	// namespace capacity
	uint64_t ncap;

	// namespace utilization
	uint64_t nuse;

	// namespace features
	uint8_t  nsfeat;

	// number of lba formats
	uint8_t  nlbaf;

	// formatted lba size
	uint8_t  flbas;

	// metadata capabilities
	uint8_t  mc;

	// end-to-end data protection capabilities
	uint8_t  dpc;

	// end-to-end data protection type settings
	uint8_t  dps;

	// namespace multi-path i/o and namespace sharing capabilities
	uint8_t  nmic;

	// reservation capabilities
	uint8_t  rescap;

	// format progress indicator
	uint8_t  fpi;

	// deallocate logical block features
	uint8_t  dlfeat;

	// namespace atomic write unit normal
	uint16_t nawun;

	// namespace atomic write unit power fail
	uint16_t nawupf;

	// namespace atomic compare & write unit
	uint16_t nacwu;

	// namespace atomic boundary sie normal
	uint16_t nabsn;

	// namespace atomic boundary offset
	uint16_t nabo;

	// namespace atomic boundary size power fail
	uint16_t nabspf;

	// namespace optimal io boundary
	uint16_t noiob;

	// namespace nvm capacity
	uint8_t  nvmcap[16];

	// reserved
	uint8_t  rsvd64[40];

	// namespace globally unique identifier
	uint8_t  nguid[16];

	// ieee extended unique identifier
	uint8_t  eui64[8];

	// lba format specifications
	struct nvm_nvme_lbaf lbaf[16];

	// reserved
	uint8_t  rsvd192[192];

	// vendor specific
	uint8_t  vs[3712];
};

#endif /* __INTERNAL_NVM_NVME_H */
