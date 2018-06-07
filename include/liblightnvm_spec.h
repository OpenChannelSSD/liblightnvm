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
	NVM_S20_OPC_RPRT = 0x02,
	NVM_S20_OPC_SFEAT = 0x09,
	NVM_S20_OPC_GFEAT = 0x0A,
	NVM_S20_OPC_ERASE = 0x90,
	NVM_S20_OPC_WRITE = 0x91,
	NVM_S20_OPC_READ = 0x92,
	NVM_S20_OPC_COPY = 0x93,
};

enum nvm_spec_opcodes {
	NVM_OPC_IDFY = 0xE2,

	NVM_OPC_RPRT = 0x02,

	NVM_OPC_SFEAT = 0x09,
	NVM_OPC_GFEAT = 0x0A,

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
	uint8_t pugrp;
	uint8_t punit;
	uint8_t chunk;
	uint8_t sectr;
	uint8_t rsvd[4];
};

struct nvm_spec_lbaz {
	uint64_t pugrp;
	uint64_t punit;
	uint64_t chunk;
	uint64_t sectr;
};

struct nvm_spec_lbam {
	uint64_t pugrp;
	uint64_t punit;
	uint64_t chunk;
	uint64_t sectr;
};

void nvm_spec_lbaf_pr(const struct nvm_spec_lbaf *lbaf);
void nvm_spec_lbaz_pr(const struct nvm_spec_lbaz *lbaz);
void nvm_spec_lbam_pr(const struct nvm_spec_lbam *lbam);

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
	uint32_t	_fna_nbytes;	///< # bytes in sector
	uint32_t	_fna_nbytes_oob;///< # bytes in sector out-of-band area
	uint8_t		resv[44];
};

void nvm_spec_lgeo_pr(const struct nvm_spec_lgeo *lgeo);

struct nvm_spec_wrt {
	uint32_t	ws_min;
	uint32_t	ws_opt;
	uint32_t	mw_cunits;
	uint32_t	maxoc;
	uint32_t	maxocpu;
	uint8_t		resv[44];
};

void nvm_spec_wrt_pr(const struct nvm_spec_wrt *lbaf);

struct nvm_spec_perf {
	uint32_t	trdt;
	uint32_t	trdm;
	uint32_t	twrt;
	uint32_t	twrm;
	uint32_t	tcet;	/* AKA tcrst */
	uint32_t	tcem;	/* AKA tcrsm */
	uint8_t		resv[40];
};

struct nvm_spec_idfy_s20 {
	uint8_t				verid;
	uint8_t				verid_minor;
	uint8_t				rsvd1[6];
	struct nvm_spec_lbaf		lbaf;
	uint32_t			mccap;
	uint8_t				rsvd2[12];
	uint8_t				wit;
	uint8_t				rsvd3[31];
	struct nvm_spec_lgeo		lgeo;
	struct nvm_spec_wrt		wrt;
	struct nvm_spec_perf		perf;
	uint8_t				rsvd4[2816];
	uint8_t				vndr[1024];
};
static_assert(sizeof(struct nvm_spec_idfy_s20) == 4096, "Incorrect size");

/**
 * Identify command data structure
 */
struct nvm_spec_idfy {
	union {
		struct nvm_spec_idfy_s12 s12;
		struct nvm_spec_idfy_s20 s20;

		struct {
			uint8_t verid;
			uint8_t verid_minor;
			uint8_t rsvd[4094];
		} s;	///< Shared between the revisions
	};
};
static_assert(sizeof(struct nvm_spec_idfy) == 4096, "Incorrect size");

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

/**
 * Representation of Spec. 2.0 chunk state (see figure 16)
 */
enum nvm_spec_chunk_state {
	NVM_CHUNK_STATE_FREE	= 0x1 << 0,
	NVM_CHUNK_STATE_CLOSED	= 0x1 << 1,
	NVM_CHUNK_STATE_OPEN	= 0x1 << 2,
	NVM_CHUNK_STATE_OFFLINE	= 0x1 << 3,
	NVM_CHUNK_STATE_RSVD4	= 0x1 << 4,
	NVM_CHUNK_STATE_RSVD5	= 0x1 << 5,
	NVM_CHUNK_STATE_RSVD6	= 0x1 << 6,
	NVM_CHUNK_STATE_RSVD7	= 0x1 << 7,
};

/**
 * Representation of Spec. 2.0 chunk type (see figure 16)
 */
enum nvm_spec_chunk_type {
	NVM_CHUNK_TYPE_SEQR	= 0x1 << 0,
	NVM_CHUNK_TYPE_ARWR	= 0x1 << 1,
	NVM_CHUNK_TYPE_RSVD2	= 0x1 << 2,
	NVM_CHUNK_TYPE_RSVD3	= 0x1 << 3,
	NVM_CHUNK_TYPE_WAVVY	= 0x1 << 4,
	NVM_CHUNK_TYPE_RSVD5	= 0x1 << 5,
	NVM_CHUNK_TYPE_RSVD6	= 0x1 << 6,
	NVM_CHUNK_TYPE_RSVD7	= 0x1 << 7,
};

/**
 * Representation of the chunk descriptor in the report chunk state table
 */
struct nvm_spec_rprt_descr {
	uint8_t cs;		///< Chunk State (CS)
	uint8_t ct;		///< Chunk Type (CT)
	uint8_t wli;		///< Wear-level Index (WLI) 0-255
	uint8_t rsvd1[5];
	uint64_t addr;		///< AKA Starting LBA (SLBA)
	uint64_t naddrs;	///< AKA Number of blocks in chunk (CNLB)
	uint64_t wp;		///< Write Pointer (WP)
};
static_assert(sizeof(struct nvm_spec_rprt_descr) == 32, "Incorrect size");

/**
 * Representation of spec. 2.0 Get Log Page for chunk information
 */
struct nvm_spec_rprt {
	uint32_t ndescr;			///< # Chunk descriptors in rprt
	struct nvm_spec_rprt_descr descr[];	///< Chunk descriptors
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
	uint16_t opcode	:  8;			///< opcode
	uint16_t fuse	:  2;			///< fused operation
	uint16_t rsvd	:  4;
	uint16_t psdt	:  2;
	uint16_t cid;				///< command identifier

	/* cdw 01 */
	uint32_t nsid;				///< namespace identifier

	uint32_t cdw02;
	uint32_t cdw03;

	/* cdw 04-05 */
	uint64_t mptr;				///< MPTR -- metadata pointer

	/* cdw 06-09: */			///< DPTR -- data pointer
	union {
		struct {
			uint64_t prp1;		///< PRP entry 1
			uint64_t prp2;		///< PRP entry 2
		} prp;
	} dptr;

	/* cdw 10-11 */
	union {
		uint64_t addrs;			///< PPA / LBA list

		struct {
			uint32_t lid	: 8;	///< Log Page Identifier
			uint32_t lsp	: 4;	///< Log Specific Field
			uint32_t rsvd10	: 3;
			uint32_t rae	: 1;	///< Retain Async. Event
			uint32_t numdl	: 16;

			uint32_t numdu	: 16;
			uint32_t rsvd11	: 16;
		} rprt;				///< RPRT accessor
	};

	/* cdw 12 */
	union {
		struct {
			uint32_t naddrs	: 16;	///< # addrs. in PPA list
			uint32_t control: 16;	///< PMODE, Scrabler, etc.
		} s12;

		struct {
			uint32_t naddrs	: 6;	///< # addrs. in LBA list
			uint32_t rsvd	: 24;
			uint32_t fua	: 1;	///< Force unit access
			uint32_t lr	: 1;	///< Limited retry
		} ewrc;

		uint32_t lpol;			///< Log Page Offset Lower

		uint32_t cdw12;			///< Accessor for arbitrary use
	};

	union {
		uint32_t lpou;			///< Log Page Offset Upper

		uint32_t cdw13;			///< Accessor for arbitrary use
	};

	/* cdw 14-15 */
	uint64_t addrs_dst;			///< destination addresses
};
static_assert(sizeof(struct nvm_nvme_cmd) == 64, "Incorrect size");

#endif /* __LIBLIGHTNVM_SPEC_H */
