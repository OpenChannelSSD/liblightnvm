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

void nvm_spec_lgeo_pr(const struct nvm_spec_lgeo *lgeo)
{
	printf("lgeo:");
	
	if (!lgeo) {
		printf(" ~\n");
		return;
	}

	printf("\n");
	printf("  npugrp: %u\n", lgeo->npugrp);
	printf("  npunit: %u\n", lgeo->npunit);
	printf("  nchunk: %u\n", lgeo->nchunk);
	printf("  nsectr: %u\n", lgeo->nsectr);
	printf("  nbytes: %u\n", lgeo->nbytes);
	printf("  nbytes_oob: %u\n", lgeo->nbytes_oob);
}

void nvm_spec_wrt_pr(const struct nvm_spec_wrt *wrt)
{
	printf("wrt:");

	if (!wrt) {
		printf(" ~\n");
		return;
	}

	printf(" {ws_min: %u, ws_opt: %u, mw_cunits: %u}\n",
	       wrt->ws_min, wrt->ws_opt, wrt->mw_cunits);
}

void nvm_spec_perf_pr(const struct nvm_spec_perf *perf)
{
	printf("perf:");

	if (!perf) {
		printf(" ~\n");
		return;
	}

	printf(" {trdt: %u, trdm: %u, twrt: %u,"
	       " twrm: %u, tcet: %u, tcem: %u}\n",
	       perf->trdt, perf->trdm, perf->twrt,
	       perf->twrm, perf->tcet, perf->tcem);
}

void nvm_spec_ppaf_nand_pr(const struct nvm_spec_ppaf_nand *ppaf)
{
	printf("ppaf:");

	if (!ppaf) {
		printf(" ~\n");
		return;
	}

	printf("\n");
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
	printf("ppaf_mask:");

	if (!mask) {
		printf(" ~\n");
		return;
	}

	printf("\n");
	printf("  ch:  '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.ch));
	printf("  lun: '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.lun));
	printf("  pl:  '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.pl));
	printf("  blk: '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.blk));
	printf("  pg:  '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.pg));
	printf("  sec: '"NVM_I64_FMT"'\n", NVM_I64_TO_STR(mask->n.sec));
}

void nvm_spec_lbaf_pr(const struct nvm_spec_lbaf *lbaf)
{
	printf("lbaf:");

	if (!lbaf) {
		printf(" ~\n");
		return;
	}

	printf("\n");
	printf("  pugrp_len: %u\n", lbaf->pugrp_len);
	printf("  punit_len: %u\n", lbaf->punit_len);
	printf("  chunk_len: %u\n", lbaf->chunk_len);
	printf("  sectr_len: %u\n", lbaf->sectr_len);
}

static void nvm_spec_idfy_s12_pr(const struct nvm_spec_idfy *identify)
{
	struct nvm_spec_idfy_s12 idfy = identify->s12;

	printf("idfy:\n");
	printf("  verid: "NVM_I8_FMT"\n", NVM_I8_TO_STR(idfy.verid));
	printf("  vnvmt(%u)\n", idfy.vnvmt);
	printf("  cgroups(%u),\n", idfy.cgroups);
	printf("  cap("NVM_I32_FMT"),\n", NVM_I32_TO_STR(idfy.cap));
	printf("  dom("NVM_I32_FMT"),\n", NVM_I32_TO_STR(idfy.dom));

	printf("spec_identify_cgrps:\n");

	for (int i = 0; i < idfy.cgroups; ++i) {
		struct nvm_spec_idfy_cgrp grp = idfy.grp[i];

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

static void nvm_spec_idfy_s13_pr(const struct nvm_spec_idfy *identify)
{
	struct nvm_spec_idfy_s13 idfy = identify->s13;

	printf("idfy:\n");
	printf("  verid: "NVM_I8_FMT"\n", NVM_I8_TO_STR(idfy.verid));
	printf("  verid_minor: "NVM_I8_FMT"\n", NVM_I8_TO_STR(idfy.verid_minor));
	printf("  mccap: "NVM_I32_FMT"\n", NVM_I32_TO_STR(idfy.mccap));
	nvm_spec_ppaf_nand_pr(&idfy.ppaf);
	nvm_spec_lbaf_pr(&idfy.lbaf);
	nvm_spec_lgeo_pr(&idfy.lgeo);
	nvm_spec_wrt_pr(&idfy.wrt);
	nvm_spec_perf_pr(&idfy.perf);
}

static void nvm_spec_idfy_s20_pr(const struct nvm_spec_idfy *identify)
{
	struct nvm_spec_idfy_s20 idfy = identify->s20;

	printf("idfy:\n");
	printf("  verid: "NVM_I8_FMT"\n", NVM_I8_TO_STR(idfy.verid));
	printf("  verid_minor: "NVM_I8_FMT"\n", NVM_I8_TO_STR(idfy.verid_minor));
	printf("  mccap: "NVM_I32_FMT"\n", NVM_I32_TO_STR(idfy.mccap));
	nvm_spec_lbaf_pr(&idfy.lbaf);
	nvm_spec_lgeo_pr(&idfy.lgeo);
	nvm_spec_wrt_pr(&idfy.wrt);
	nvm_spec_perf_pr(&idfy.perf);
}

void nvm_spec_idfy_pr(const struct nvm_spec_idfy *idfy)
{
	if (!idfy) {
		printf("nvm_spec_idfy: ~\n");
		return;
	}

	switch(idfy->s.verid) {
	case NVM_SPEC_VERID_12:
		nvm_spec_idfy_s12_pr(idfy);
		break;

	case NVM_SPEC_VERID_13:
		nvm_spec_idfy_s13_pr(idfy);
		break;

	case NVM_SPEC_VERID_20:
		nvm_spec_idfy_s20_pr(idfy);
		break;

	default:
		printf("nvm_spec_idfy:\n");
		printf("  verid("NVM_I8_FMT"),\n", NVM_I8_TO_STR(idfy->s.verid));
	}
}

void nvm_spec_bbt_pr(const struct nvm_spec_bbt *bbt)
{
	if (!bbt) {
		printf("bbt: ~\n");
		return;
	}

	printf("bbt:\n");
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

