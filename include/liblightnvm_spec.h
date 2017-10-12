/*
 * liblightnvm_spec - Open-Channel SSD / LightNVM specfication rev. 1.2 and 2.0
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
/**
 * @file liblightnvm_spec.h
 */
#ifndef __LIBLIGHTNVM_SPEC_H
#define __LIBLIGHTNVM_SPEC_H

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>

enum nvm_spec_12_mccap {
	NVM_SPEC_12_MCCAP_SLCMODE = 0x1,
	NVM_SPEC_12_MCCAP_SUSPENSION = 0x1 << 1,
	NVM_SPEC_12_MCCAP_SCRAMBLER = 0x1 << 2,
	NVM_SPEC_12_MCCAP_ENCRYPTION = 0x1 << 3,
};

enum nvm_spec_20_mccap {
	NVM_SPEC_20_MCCAP_VCOPY = 0x1,
};

/**
 * Encoding descriptor for physical address format for NAND
 */
struct nvm_spec_ppaf_nand {
	union {
		struct {
			uint8_t ch_off;		///< Offset in bits for channel
			uint8_t ch_len;		///< Nr. of bits repr. channel
			uint8_t lun_off;	///< Offset in bits for LUN
			uint8_t lun_len;	///< Nr. of bits repr. LUN
			uint8_t pl_off;		///< Offset in bits for plane
			uint8_t pl_len;		///< Nr. of bits repr. plane
			uint8_t blk_off;	///< Offset in bits for block
			uint8_t blk_len;	///< Nr. of bits repr. block
			uint8_t pg_off;		///< Offset in bits for page
			uint8_t pg_len;		///< Nr. of bits repr. page
			uint8_t sec_off;	///< Offset in bits for sector
			uint8_t sec_len;	///< Nr. of bits repr. sector
			uint8_t rsvd[4];
		} n;

		/**
		* Address format formed as anonymous consecutive fields
		*/
		uint8_t a[16];
	};
};


/**
 * Opcodes, enums, and data structures representing definitions in
 *
 * - Open-Channel Solid State Drives NVMe Specification Revision 1.2
 * - Open-Channel Solid State Drives NVMe Specification Revision 2.0
 */
enum nvm_spec_12_opcodes {
	NVM_S12_OPC_IDF = 0xE2,
	NVM_S12_OPC_SET_BBT = 0xF1,
	NVM_S12_OPC_GET_BBT = 0xF2,
	NVM_S12_OPC_ERASE = 0x90,
	NVM_S12_OPC_WRITE = 0x91,
	NVM_S12_OPC_READ = 0x92,
};

enum nvm_spec_20_opcodes {
	NVM_S20_OPC_IDF = 0xE2,
	NVM_S20_OPC_RPT = 0xF2,
	NVM_S20_OPC_ERASE = 0x90,
	NVM_S20_OPC_WRITE = 0x91,
	NVM_S20_OPC_READ = 0x92,
	NVM_S20_OPC_COPY = 0x93,
};

enum nvm_spec_opcodes {
	NVM_OPC_IDFY = 0xE2,
	NVM_OPC_STATE = 0xF2,
	NVM_OPC_ERASE = 0x90,
	NVM_OPC_WRITE = 0x91,
	NVM_OPC_READ = 0x92,
	NVM_OPC_COPY = 0x93
};

/**
 * Specification version identifier
 */
enum nvm_spec_verid {
	NVM_SPEC_VERID_12 = 0x1,
	NVM_SPEC_VERID_13 = 0x2 + (0x1 << 7),
	NVM_SPEC_VERID_20 = 0x2,
};

/**
 * Masks for removing bit information from a formatted address
 *
 * E.g. "addr & mask.n.ch" returns the bits representing the channel,
 * this value can then be shifted to obtain the numerical value.
 */
struct nvm_spec_ppaf_nand_mask {
	union {
		/**
		 * Address format masks formed as named fields
		 */
		struct {
			uint64_t ch;	///< Mask for channel bits
			uint64_t lun;	///< Mask for lun bits
			uint64_t pl;	///< Mask for pl bits
			uint64_t blk;	///< Mask for blk bits
			uint64_t pg;	///< Mask for page bits
			uint64_t sec;	///< Mask for sector bits
		} n;

		/**
		 * Address format formed as anonymous consecutive fields
		 */
		uint64_t a[6];
	};
};

struct nvm_spec_lbaf {
	uint8_t pugrp_len;
	uint8_t punit_len;
	uint8_t chunk_len;
	uint8_t sectr_len;
	uint8_t rsvd[4];
};

void nvm_spec_lbaf_pr(const struct nvm_spec_lbaf *lbaf);

struct nvm_spec_idfy_cgrp {
	uint8_t mtype;
	uint8_t fmtype;
	uint8_t rsvd_2_3[2];
	uint8_t num_ch;
	uint8_t num_lun;
	uint8_t num_pln;
	uint8_t rsdv_7;
	uint16_t num_blk;
	uint16_t num_pg;
	uint16_t fpg_sz;
	uint16_t csecs;
	uint16_t sos;
	uint8_t rsvd_18_19[2];
	uint32_t trdt;
	uint32_t trdm;
	uint32_t tprt;
	uint32_t tprm;
	uint32_t tbet;
	uint32_t tbem;
	uint32_t mpos;
	uint32_t mccap;
	uint16_t cpar;
	uint8_t rsvd_54_63[10];
	uint8_t mts[896];
};

struct nvm_spec_idfy_s12 {
	uint8_t verid;
	uint8_t vnvmt;
	uint8_t cgroups;
	uint8_t rsvd3;
	uint32_t cap;
	uint32_t dom;
	struct nvm_spec_ppaf_nand ppaf;
	uint8_t rsdv_028_255[228];
	struct nvm_spec_idfy_cgrp grp[4];
};

struct nvm_spec_lgeo {
	uint16_t	npugrp;		///< # controller groups (GRP)
	uint16_t	npunit;		///< # parallel units (PUN) per GRP
	uint32_t	nchunk;		///< # chunks (CHK) per PUN
	uint32_t	nsectr;		///< # sectors in chunk
	uint32_t	nbytes;		///< # bytes in sector
	uint32_t	nbytes_oob;	///< # bytes in sector out-of-band area
	uint8_t		resv[44];
};

void nvm_spec_lgeo_pr(const struct nvm_spec_lgeo *lgeo);

struct nvm_spec_wrt {
	uint32_t	ws_min;
	uint32_t	ws_opt;
	uint32_t	mw_cunits;
	uint8_t		resv[52];
};

void nvm_spec_wrt_pr(const struct nvm_spec_wrt *lbaf);

struct nvm_spec_perf {
	uint32_t	trdt;
	uint32_t	trdm;
	uint32_t	twrt;
	uint32_t	twrm;
	uint32_t	tcet;
	uint32_t	tcem;
	uint8_t		resv[40];
};

struct nvm_spec_idfy_s13 {
	uint8_t				verid;
	uint8_t				verid_minor;
	uint8_t				rsvd1[2];
	struct nvm_spec_lbaf		lbaf;
	struct nvm_spec_ppaf_nand	ppaf;
	uint8_t				rsvd2[4];
	uint32_t			mccap;
	uint8_t				rsvd3[28];
	struct nvm_spec_lgeo		lgeo;
	struct nvm_spec_wrt		wrt;
	struct nvm_spec_perf	perf;
	uint8_t				rsvd4;
};

struct nvm_spec_idfy_s20 {
	uint8_t				verid;
	uint8_t				verid_minor;
	uint8_t				rsvd1[6];
	struct nvm_spec_lbaf		lbaf;
	uint32_t			mccap;
	uint8_t				rsvd2[44];
	struct nvm_spec_lgeo		lgeo;
	struct nvm_spec_wrt		wrt;
	struct nvm_spec_perf	perf;
};

/**
 * Identify command data structure
 */
struct nvm_spec_idfy {
	union {
		struct nvm_spec_idfy_s12 s12;
		struct nvm_spec_idfy_s13 s13;
		struct nvm_spec_idfy_s20 s20;

		struct {
			uint8_t verid;
			uint8_t rsvd[4095];
		} s;	///< Shared between the revisions
	};
};

void nvm_spec_idfy_pr(const struct nvm_spec_idfy *idfy);

struct nvm_spec_bbt {
	uint8_t		tblid[4];
	uint16_t	verid;
	uint16_t	revid;
	uint32_t	rvsd1;
	uint32_t	tblks;
	uint32_t	tfact;
	uint32_t	tgrown;
	uint32_t	tdresv;
	uint32_t	thresv;
	uint32_t	rsvd2[8];
	uint8_t		blk[];
};

enum nvm_spec_rptr_opts {
	NVM_RPRT_ALL = 0x0,
	NVM_RPRT_FREE = 0x1,
	NVM_RPRT_FULL = 0x2,
	NVM_RPRT_OPEN = 0x3,
	NVM_RPRT_BAD = 0x4,

	NVM_RPRT_SEQW = 0xA,
	NVM_RPRT_ARBW = 0xB,
};

/**
 * Representation of the chunk descriptor in the report chunk state table
 */
struct nvm_spec_rprt_descr {
	uint8_t chunk_state;
	uint8_t chunk_type;
	uint8_t chunk_limits;
	uint8_t rsvd1[5];
	uint64_t chunk_addr;
	uint64_t chunk_naddrs;
	uint64_t chunk_wptr;
	uint8_t rsvd2[32];
};

/**
 * Representation of the chunk state table returned from the report chunk
 * command
 */
struct nvm_spec_rprt {
	uint64_t nchunks;			///< #chunks in report
	uint8_t rsvd[56];
	struct nvm_spec_rprt_descr descr[];	///< Chunk descriptor table
};

void nvm_spec_rprt_pr(const struct nvm_spec_rprt *rprt);

/**
 * Prints a humanly readable representation of the give address format mask
 *
 * @param ppaf The address format to print
 */
void nvm_spec_ppaf_nand_pr(const struct nvm_spec_ppaf_nand *ppaf);

/**
 * Prints a humanly readable representation of the give address format mask
 *
 * @param fmt The address format mask to print
 */
void nvm_spec_ppaf_nand_mask_pr(const struct nvm_spec_ppaf_nand_mask *mask);

/**
 * Prints a humanly readable representation of the given spec_bbt
 *
 * @param ppaf The address format to print
 */
void nvm_spec_bbt_pr(const struct nvm_spec_bbt *bbt);

struct nvm_nvme_cmd {
	/* cdw 00 */
	uint16_t opcode	:  8;			/* opcode */
	uint16_t fuse	:  2;			/* fused operation */
	uint16_t rsvd	:  4;
	uint16_t psdt	:  2;
	uint16_t cid;				/* command identifier */

	/* cdw 01 */
	uint32_t nsid;				/* namespace identifier */

	uint32_t cdw02;
	uint32_t cdw03;

	/* cdw 04-05 */
	uint64_t mptr;				/* MPTR -- metadata pointer */

	/* cdw 06-09: */			/* DPTR -- data pointer */
	union {
		struct {
			uint64_t prp1;		/* PRP entry 1 */
			uint64_t prp2;		/* PRP entry 2 */
		} prp;
	} dptr;

	/* cdw 10-11 */
	uint64_t addrs;				/* PPA / LBA list */

	/* cdw 12 */
	union {
		struct {
			uint32_t naddrs	: 16;	/* # addrs. in PPA list */
			uint32_t control: 16;	/* PMODE, Scrabler, etc. */
		} s12;

		struct {
			uint32_t naddrs	: 6;	/* # addrs. in LBA list */
			uint32_t rsvd	: 24;
			uint32_t fua	: 1;	/* Force unit access */
			uint32_t lr	: 1;	/* Limited retry */
		} ewrc;

		struct {
			uint32_t naddrs	: 16;	/* # chunks to report */
			uint32_t ro	: 4;	/* Reporting options */
			uint32_t rsvd	: 12;	/* Reporting options */
		} rprt;
	};

	uint32_t cdw13;

	/* cdw 14-15 */
	uint64_t addrs_dst;			/* destination addresses */
};
static_assert(sizeof(struct nvm_nvme_cmd) == 64, "Incorrect size");

#endif /* __LIBLIGHTNVM_SPEC_H */
