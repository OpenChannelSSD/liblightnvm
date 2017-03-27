/*
 * nvm_spec - Printers and helper functions
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
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_be.h>
#include <nvm_dev.h>
#include <liblightnvm_spec.h>
#include <nvm_utils.h>
#include <nvm_debug.h>

void _s12_identify_pr(const struct nvm_spec_identify *identify)
{
	struct s12_identify idf = identify->s12;

	printf("nvm_spec_identify:\n");
	printf("  verid: "NVM_I8_FMT"\n", NVM_I8_TO_STR(idf.verid));
	printf("  vnvmt(%u)\n", idf.vnvmt);
	printf("  cgroups(%u),\n", idf.cgroups);
	printf("  cap("NVM_I32_FMT"),\n", NVM_I32_TO_STR(idf.cap));
	printf("  dom("NVM_I32_FMT"),\n", NVM_I32_TO_STR(idf.dom));

	printf("spec_identify_cgrps:\n");

	for (int i = 0; i < idf.cgroups; ++i) {
		struct spec_cgrp grp = idf.grp[i];

		printf("  mtype: 0x%02x)\n", grp.mtype);
		if (grp.mtype)
			continue;

		printf("  fmtype: "NVM_I8_FMT"\n", NVM_I8_TO_STR(grp.fmtype));
		printf("  num_ch: %u\n", grp.num_ch);
		printf("  num_luns: %u\n", grp.num_lun);
		printf("  num_pln: %u\n", grp.num_pln);
		printf("  num_blk: %u\n", grp.num_blk);
		printf("  num_pg: %u\n", grp.num_pg);
		printf("  fpg_sz: %u\n", grp.fpg_sz);
		printf("  csecs: %u\n", grp.csecs);
		printf("  sos: %u\n", grp.sos);
		printf("  trdt: %d\n", grp.trdt);
		printf("  trdm: %d\n", grp.trdm);
		printf("  tprt: %d\n", grp.tprt);
		printf("  tprm: %d\n", grp.tprm);
		printf("  tbet: %d\n", grp.tbet);
		printf("  tbem: %d\n", grp.tbem);
		printf("  mpos: "NVM_I32_FMT"\n", NVM_I32_TO_STR(grp.mpos));
		printf("  mccap: "NVM_I32_FMT"\n", NVM_I32_TO_STR(grp.mccap));
		printf("  cpar: %d\n", grp.cpar);
		printf("  mts: NOT_IMPLEMENTED\n");
	}
}

void _s20_identify_pr(const struct nvm_spec_identify *identify)
{
	struct s20_identify idf = identify->s20;

	printf("nvm_spec_identify:\n");
	printf("  verid: "NVM_I8_FMT"\n", NVM_I8_TO_STR(idf.verid));
	printf("  mccap: "NVM_I32_FMT"\n", NVM_I32_TO_STR(idf.mccap));
	printf("  num_ch: %u\n", idf.num_ch);
	printf("  num_lun: %u\n", idf.num_lun);
	printf("  num_chk: %u\n", idf.num_chk);
	printf("  clba: %u\n", idf.clba);
	printf("  csecs: %u\n", idf.csecs);
	printf("  sos: %u\n", idf.sos);
	printf("  mw_min: %u\n", idf.mw_min);
	printf("  mw_opt: %u\n", idf.mw_opt);
	printf("  mw_cunits: %u\n", idf.mw_cunits);
	printf("  trdt: %d\n", idf.trdt);
	printf("  trdm: %d\n", idf.trdm);
	printf("  twrt: %d\n", idf.twrt);
	printf("  twrm: %d\n", idf.twrm);
	printf("  tcet: %d\n", idf.tcet);
	printf("  tcem: %d\n", idf.tcem);
	printf("spec_identify_");
	nvm_spec_lbaf_pr(&idf.lbaf);
	printf("spec_identify_");
	nvm_spec_ppaf_nand_pr(&idf.ppaf);
}

void nvm_spec_identify_pr(const struct nvm_spec_identify *identify)
{
	if (!identify) {
		printf("nvm_spec_identify: ~\n");
		return;
	}

	switch(identify->s.verid) {
	case NVM_SPEC_VERID_12:
		_s12_identify_pr(identify);
		break;
	case NVM_SPEC_VERID_20:
		_s20_identify_pr(identify);
		break;

	default:
		printf("nvm_spec_identify:\n");
		printf("  verid("NVM_I8_FMT"),\n", NVM_I8_TO_STR(identify->s.verid));
	}
}

void nvm_spec_ppaf_nand_pr(const struct nvm_spec_ppaf_nand *ppaf)
{
	printf("ppaf:\n");
	printf("  ch_off: %02u\n", ppaf->n.ch_off);
	printf("  ch_len: %02u\n", ppaf->n.ch_len);
	printf("  lun_off: %02u\n", ppaf->n.lun_off);
	printf("  lun_len: %02u\n", ppaf->n.lun_len);
	printf("  pl_off: %02u\n", ppaf->n.pl_off);
	printf("  pl_len: %02u\n", ppaf->n.pl_len);
	printf("  blk_off: %02u\n", ppaf->n.blk_off);
	printf("  blk_len: %02u\n", ppaf->n.blk_len);
	printf("  pg_off: %02u\n", ppaf->n.pg_off);
	printf("  pg_len: %02u\n", ppaf->n.pg_len);
	printf("  sec_off: %02u\n", ppaf->n.sec_off);
	printf("  sec_len: %02u\n", ppaf->n.sec_len);
}

void nvm_spec_ppaf_nand_mask_pr(const struct nvm_spec_ppaf_nand_mask *mask)
{
	printf("ppaf_mask:\n");
	printf("  ch:  '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.ch));
	printf("  lun: '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.lun));
	printf("  pl:  '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.pl));
	printf("  blk: '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.blk));
	printf("  pg:  '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.pg));
	printf("  sec: '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.sec));
}

void nvm_spec_lbaf_pr(const struct nvm_spec_lbaf *lbaf)
{
	printf("lbaf:\n");
	printf("  ch_len: %u\n", lbaf->ch_len);
	printf("  lun_len: %u\n", lbaf->lun_len);
	printf("  cnk_len: %u\n", lbaf->cnk_len);
	printf("  sec_len: %u\n", lbaf->sec_len);
}

void nvm_spec_bbt_pr(const struct nvm_spec_bbt *bbt)
{
	if (!bbt) {
		printf("nvm_spec_bbt: ~\n");
		return;
	}

	printf("nvm_spec_bbt:\n");
	printf("  tblkid: %c%c%c%c\n",
		   bbt->tblid[0], bbt->tblid[1], bbt->tblid[2], bbt->tblid[3]);
	printf("  verid: %u\n", bbt->verid);
	printf("  revid: %u\n", bbt->revid);
	printf("  rvsd1: %u\n", bbt->rvsd1);
	printf("  tblks: %u\n", bbt->tblks);
	printf("  tfact: %u\n", bbt->tfact);
	printf("  tgrown: %u\n", bbt->tgrown);
	printf("  tdresv: %u\n", bbt->tdresv);
	printf("  thresv: %u\n", bbt->thresv);
	printf("  rsvd2: ~\n");

	printf("  tblks:\n");
	for (uint32_t i = 0; i < bbt->tblks; ++i) {
		printf("    - %u\n", bbt->blk[i]);
	}
}

struct nvm_spec_bbt *nvm_spec_bbt_get(struct nvm_dev *dev, struct nvm_addr addr,
				      struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	struct nvm_spec_bbt *spec_bbt;
	size_t spec_bbt_sz;
	int err;

	uint32_t nblks = dev->geo.nblocks * dev->geo.nplanes;
	spec_bbt_sz = sizeof(*spec_bbt) + sizeof(*(spec_bbt->blk)) * nblks;
	spec_bbt = nvm_buf_alloc(&dev->geo, spec_bbt_sz);
	if (!spec_bbt) {
		NVM_DEBUG("FAILED: malloc k_bbt failed");
		errno = ENOMEM;
		return NULL;
	}

	cmd.vadmin.opcode = NVM_S12_OPC_GET_BBT;
	cmd.vadmin.addr = (uint64_t)spec_bbt;
	cmd.vadmin.data_len = spec_bbt_sz;
	cmd.vadmin.ppa_list = nvm_addr_gen2dev(dev, addr);
	cmd.vadmin.nppas = 0;

	err = dev->be->vadmin(dev, &cmd, ret);
	if (err || (spec_bbt->tblks != nblks)) {
		NVM_DEBUG("FAILED: be execution failed");
		errno = EIO;
		free(spec_bbt);
		return NULL;
	}
	if (!(spec_bbt->tblid[0] == 'B' && spec_bbt->tblid[1] == 'B' &&
	      spec_bbt->tblid[2] == 'L' && spec_bbt->tblid[3] == 'T')) {
		NVM_DEBUG("FAILED: invalid format of returned bbt");
		errno = EIO;
		free(spec_bbt);
		return NULL;
	}

	return spec_bbt;
}

int nvm_spec_bbt_set(struct nvm_dev *dev, struct nvm_addr *addrs,
		     int naddrs, uint16_t flags, struct nvm_ret *ret)
{
	struct nvm_cmd cmd = {.cdw={0}};
	uint64_t dev_addrs[naddrs];
	int err;

	switch(flags) {
	case NVM_BBT_FREE:
	case NVM_BBT_BAD:
	case NVM_BBT_DMRK:
	case NVM_BBT_GBAD:
	case NVM_BBT_HMRK:
		break;
	default:
		NVM_DEBUG("FAILED: invalid mark");
		errno = EINVAL;
		return -1;
	}

	if (naddrs > NVM_NADDR_MAX) {
		NVM_DEBUG("FAILED: naddrs > NVM_NADDR_MAX");
		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < naddrs; ++i) {	// Setup PPAs: Convert format
		if (nvm_addr_check(addrs[i], &dev->geo)) {
			NVM_DEBUG("FAILED: invalid addrs[i]");
			errno = EINVAL;
			return -1;
		}
		dev_addrs[i] = nvm_addr_gen2dev(dev, addrs[i]);
	}

	cmd.vadmin.opcode = NVM_S12_OPC_SET_BBT;	// Construct command
	cmd.vadmin.control = flags;
	cmd.vadmin.nppas = naddrs - 1;		// Unnatural numbers: counting from zero
	cmd.vadmin.ppa_list = naddrs == 1 ? dev_addrs[0] : (uint64_t)dev_addrs;

	err = dev->be->vadmin(dev, &cmd, ret);
	if (err) {
		NVM_DEBUG("FAILED: be execution failed");
		errno = EIO;
		return -1;
	}

	return 0;
}

