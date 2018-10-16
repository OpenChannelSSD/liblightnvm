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
 * OCSSD 1.2/2.0 admin command opcodes
 */
enum nvm_adm_opcodes {
	NVM_AOPC_IDFY = 0xE2,
	NVM_AOPC_RPRT = 0x02,
	NVM_AOPC_SFEAT = 0x09,
	NVM_AOPC_GFEAT = 0x0A,

	NVM_AOPC_SBBT = 0xF1,
	NVM_AOPC_GBBT = 0xF2,
};

/**
 * OCSSD 1.2/2.0 data-set management and IO command opcodes
 */
enum nvm_dio_opcodes {
	NVM_DOPC_SCALAR_ERASE = 0x09,
	NVM_DOPC_SCALAR_WRITE = 0x01,
	NVM_DOPC_SCALAR_READ = 0x02,

	NVM_DOPC_VECTOR_ERASE = 0x90,
	NVM_DOPC_VECTOR_WRITE = 0x91,
	NVM_DOPC_VECTOR_READ = 0x92,
	NVM_DOPC_VECTOR_COPY = 0x93
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
	uint8_t		resv[52];
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

void nvm_spec_idfy_pr(const struct nvm_spec_idfy *idfy, int quirks);

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

void nvm_spec_rprt_descr_pr(const struct nvm_spec_rprt_descr *descr);

/**
 * Representation of spec. 2.0 Get Log Page for chunk information
 */
struct nvm_spec_rprt {
	uint32_t ndescr;			///< # Chunk descriptors in rprt
	struct nvm_spec_rprt_descr descr[];	///< Chunk descriptors
};

void nvm_spec_rprt_pr(const struct nvm_spec_rprt *rprt);

/**
 * Representation of Spec. 2.0 feature types
 */
enum nvm_nvme_feat_id {
	NVM_FEAT_ERROR_RECOVERY = 0x5,
	NVM_FEAT_MEDIA_FEEDBACK = 0xCA
};

/**
 * Encapsulation of NVMe/NVM features.
 *
 * @union nvm_nvme_feat
 */
union nvm_nvme_feat {
	/**
	 * This feature controls the error recovery attributes.
	 */
	struct {
		uint32_t tler  : 16;
		uint32_t dulbe :  1;
		uint32_t rsvd  : 15;
	} error_recovery;

	/**
	 * This feature controls read command feedback.
	 */
	struct {
		uint32_t hecc  : 1;
		uint32_t vhecc : 1;
		uint32_t rsvd  : 30;
	} media_feedback;

	/**
	* Address format formed as anonymous consecutive fields
	*/
	uint32_t a;
};
static_assert(sizeof(union nvm_nvme_feat) == 4, "Incorrect size");

struct nvm_nvme_lbaf {
	uint16_t ms;	///< metadata size
	uint8_t  ds;	///< lba data size
	uint8_t  rp;	///< relative performance
};
static_assert(sizeof(struct nvm_nvme_lbaf) == 4, "Incorrect size");

struct nvm_nvme_ns {
	uint64_t nsze;			///< namespace size
	uint64_t ncap;			///< namespace capacity
	uint64_t nuse;			///< namespace utilization
	uint8_t  nsfeat;		///< namespace features
	uint8_t  nlbaf;			///< number of lba formats
	uint8_t  flbas;			///< formatted lba size
	uint8_t  mc;			///< metadata capabilities
	uint8_t  dpc;			///< end-to-end data protection
					///  capabilities
	uint8_t  dps;			///< end-to-end data protection type
					///  settings
	uint8_t  nmic;			///< namespace multi-path i/o and
					///  namespace sharing capabilities
	uint8_t  rescap;		///< reservation capabilities
	uint8_t  fpi;			///< format progress indicator
	uint8_t  dlfeat;		///< deallocate logical block features
	uint16_t nawun;			///< namespace atomic write unit normal
	uint16_t nawupf;		///< namespace atomic write unit power
					///  fail
	uint16_t nacwu;			///< namespace atomic compare & write
					///  unit
	uint16_t nabsn;			///< namespace atomic boundary size
					///  normal
	uint16_t nabo;			///< namespace atomic boundary offset
	uint16_t nabspf;		///< namespace atomic boundary size
					///  power fail
	uint16_t noiob;			///< namespace optimal io boundary
	uint8_t  nvmcap[16];		///< namespace nvm capacity
	uint8_t  rsvd64[40];		///< reserved
	uint8_t  nguid[16];		///< namespace globally unique
					///  identifier
	uint8_t  eui64[8];		///< ieee extended unique identifier
	struct nvm_nvme_lbaf lbaf[16];	///< lba format specifications
	uint8_t  rsvd192[192];		///< reserved
	uint8_t  vs[3712];		///< vendor specific
};
static_assert(sizeof(struct nvm_nvme_ns) == 4096, "Incorrect size");

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

/**
 * Representation of DSM ranges as defined in NVM Express 1.3 Figure 207 Dataset
 * Management Range Definition Figure 207
 */
struct nvm_nvme_dsm_range {
	uint32_t	cattr;	///< Context attributes
	uint32_t	nlb;	///< Length in logical blocks
	uint64_t	slba;	///< Starting LBA
};

/**
 * Enumeration of NVMe flags
 */
enum nvm_nvme_flag {
	// Limited Retry (LR)
	NVM_NVME_FLAG_LIMITED_RETRY		= 0x1 << 15,

	// Force Unit Access (FUA)
	NVM_NVME_FLAG_FORCE_UNIT_ACCESS		= 0x1 << 14,

	// Protection Information Check (PRCK)
	NVM_NVME_FLAG_PRINFO_PRCHK_REF		= 0x1 << 10,
	NVM_NVME_FLAG_PRINFO_PRCHK_APP		= 0x1 << 11,
	NVM_NVME_FLAG_PRINFO_PRCHK_GUARD	= 0x1 << 12,

	// Protection Information Action (PRACT)
	NVM_NVME_FLAG_PRINFO_PRACT		= 0x1 << 13,
};

struct nvm_nvme_status {
	uint16_t p	:  1;	///< phase tag
	uint16_t sc	:  8;	///< status code
	uint16_t sct	:  3;	///< status code type
	uint16_t rsvd2	:  2;
	uint16_t m	:  1;	///< more
	uint16_t dnr	:  1;	///< do not retry
};
static_assert(sizeof(struct nvm_nvme_status) == 2, "Incorrect size");

/**
 * Completion queue entry
 */
struct nvm_nvme_cpl {
	/* dword 0 */
	uint32_t		cdw0;	///< command-specific

	/* dword 1 */
	uint32_t		rsvd1;

	/* dword 2 */
	uint16_t		sqhd;	///< submission queue head pointer
	uint16_t		sqid;	///< submission queue identifier

	/* dword 3 */
	uint16_t		cid;	///< command identifier
	struct nvm_nvme_status	status;
};
static_assert(sizeof(struct nvm_nvme_cpl) == 16, "Incorrect size");

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
			uint32_t nr	: 8;
			uint32_t rsvd10	: 24;
			uint32_t idr	: 1;
			uint32_t idw	: 1;
			uint32_t ad	: 1;
			uint32_t rsvd11	: 29;
		} dsm;

		struct {
			uint32_t lid	: 8;	///< Log Page Identifier
			uint32_t lsp	: 4;	///< Log Specific Field
			uint32_t rsvd10	: 3;
			uint32_t rae	: 1;	///< Retain Async. Event
			uint32_t numdl	: 16;

			uint32_t numdu	: 16;
			uint32_t rsvd11	: 16;
		} rprt;				///< RPRT accessor

		struct {
			uint32_t fid    : 8;	///< Feature Identifier
			uint32_t rsvd10 : 23;
			uint32_t save   : 1;	///< Save

			union nvm_nvme_feat feat;
		} sfeat;

		struct {
			uint32_t fid    : 8;	///< Feature Identifier
			uint32_t sel	: 3;	///< Select
			uint32_t rsvd10 : 21;

			uint32_t unused11;
		} gfeat;
	};

	/* cdw 12 */
	union {
		struct {
			uint32_t naddrs : 16;	///< Number of logical blocks
			uint32_t rsvd	: 10;
			uint32_t prinfo	:  4;	///< Protection Information
						///< Field
			uint32_t fua	:  1;	///< Force unit access
			uint32_t lr	:  1;	///< Limited retry
		} nvme;

		struct {
			uint32_t naddrs	: 16;	///< # addrs. in PPA list
			uint32_t control: 16;	///< PMODE, Scrabler, etc.
		} s12;

		struct {
			uint32_t naddrs	: 6;	///< # addrs. in LBA list
			uint32_t rsvd	: 24;
			uint32_t fua	: 1;	///< Force unit access
			uint32_t lr	: 1;	///< Limited retry
		} s20;

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
