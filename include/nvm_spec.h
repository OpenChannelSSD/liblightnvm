/*
 * nvm_spec - internal header for LightNVM specfication rev. 1.2 and 2.0
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
 * Copyright (C) 2017 Simon A. F. Lund <slund@cnexlabs.com>
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
#ifndef __INTERNAL_NVM_SPEC_H
#define __INTERNAL_NVM_SPEC_H

#include <stdint.h>
#include <sys/types.h>

/**
 * Opcodes, enums, and data structures representing definitions in
 *
 * - Open-Channel Solid State Drives NVMe Specification Revision 1.2
 * - Open-Channel Solid State Drives NVMe Specification Revision 2.0
 *
 */
enum spec_12_opcodes {
	S12_OPC_IDF = 0xE2,
	S12_OPC_SET_BBT = 0xF1,
	S12_OPC_GET_BBT = 0xF2,
	S12_OPC_ERASE = 0x90,
	S12_OPC_WRITE = 0x91,
	S12_OPC_READ = 0x92,
};

enum spec_verid {
	SPEC_VERID_12 = 0x1,
	SPEC_VERID_20 = 0x2,
};

/**
 * Masks for removing bit information from a formatted address
 *
 * E.g. "addr & mask.n.ch" returns the bits representing the channel,
 * this value can then be shifted to obtain the numerical value.
 */
struct spec_ppaf_nand_mask {
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

/**
 * Encoding descriptor for physical address format for NAND
 */
struct spec_ppaf_nand {
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


struct spec_lbaf {
	uint8_t ch_len;
	uint8_t lun_len;
	uint8_t cnk_len;
	uint8_t sec_len;
	uint8_t rsvd[4];
};

struct spec_identify {
	union {
		struct s12_identify {
			uint8_t verid;
			uint8_t vnvmt;
			uint8_t cgroups;
			uint8_t rsvd3;
			uint32_t cap;
			uint32_t dom;
			struct spec_ppaf_nand ppaf;
			uint8_t rsdv_028_255[228];

			struct spec_cgrp {
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
			} grp[4];

		} s12;	///< Revision 1.2 fields

		struct s20_identify {
			uint8_t verid;
			uint8_t rsvd_001_003[3];
			struct spec_lbaf lbaf;
			struct spec_ppaf_nand ppaf;
			uint8_t rsvd_028_031[4];
			uint32_t mccap;
			uint8_t rsvd_036_063[28];
			uint16_t num_ch;
			uint16_t num_lun;
			uint32_t num_chk;
			uint32_t clba;
			uint32_t csecs;
			uint32_t sos;
			uint8_t rsvd_084_127[44];
			uint32_t mw_min;
			uint32_t mw_opt;
			uint32_t mw_cunits;
			uint8_t rsvd_140_191[52];
			uint32_t trdt;
			uint32_t trdm;
			uint32_t twrt;
			uint32_t twrm;
			uint32_t tcet;
			uint32_t tcem;
			uint8_t rsvd_216_255[41];
		} s20;	///< Revision 2.0 fields

		struct {
			uint8_t verid;
			uint8_t rsvd[4095];
		} s;	///< Shared between the revisions
	};
};

void spec_identify_pr(struct spec_identify *idf);


void spec_lbaf_pr(struct spec_lbaf *lbaf);

/**
 * Prints a humanly readable representation of the give address format mask
 *
 * @param ppaf The address format to print
 */
void spec_ppaf_nand_pr(struct spec_ppaf_nand* ppaf);

/**
 * Prints a humanly readable representation of the give address format mask
 *
 * @param fmt The address format mask to print
 */
void spec_ppaf_nand_mask_pr(struct spec_ppaf_nand_mask* mask);

struct spec_bbt {
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
 * Construct and execute a LigthNVM spec rev.1.2 BBT_GET command
 *
 * @returns 0 on success. -1 on error and errno set to indicate the error
 */
struct spec_bbt *spec_bbt_get(struct nvm_dev *dev, struct nvm_addr addr,
			      struct nvm_ret *ret);

/**
 * Construct and execute an LightNVM spec rev. 1.2 BBT_SET command
 */
int spec_bbt_set(struct nvm_dev *dev, struct nvm_addr addrs[],
		 int naddrs, uint16_t flags, struct nvm_ret *ret);

/**
 * Prints a humanly readable representation of the given spec_bbt
 *
 * @param ppaf The address format to print
 */
void spec_bbt_pr(struct spec_bbt *bbt);

#endif /* __INTERNAL_NVM_SPEC_H */
