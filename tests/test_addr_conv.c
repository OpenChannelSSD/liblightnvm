#include "test_intf.c"

static void conv_sectr_addresses(int func)
{
	size_t tsectr = 0;
	
	switch (nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		tsectr = GEO->g.nchannels * GEO->g.nluns * GEO->g.nplanes *
			GEO->g.nblocks * GEO->g.npages * GEO->g.nsectors;
		break;
	
	case NVM_SPEC_VERID_20:
		tsectr = GEO->l.npugrp * GEO->l.npunit * GEO->l.nchunk *
			 GEO->l.nsectr;
		break;

	default:
		CU_FAIL("INVALID VERID");
		return;
	}

	for (size_t sectr = 0; sectr < tsectr; ++sectr) {
		struct nvm_addr exp = { .val = 0 };
		struct nvm_addr act = { .val = 0 };
		uint64_t conv;

		switch (nvm_dev_get_verid(DEV)) {
		case NVM_SPEC_VERID_12:
			exp.g.sec = sectr % GEO->nsectors;
			exp.g.pg = (sectr / GEO->nsectors ) % GEO->npages;
			exp.g.blk = ((sectr / GEO->nsectors) / GEO->npages ) % GEO->nblocks;
			exp.g.pl = (((sectr / GEO->nsectors) / GEO->npages ) / GEO->nblocks) % GEO->nplanes;
			exp.g.lun = ((((sectr / GEO->nsectors) / GEO->npages ) / GEO->nblocks) / GEO->nplanes) % GEO->nluns;
			exp.g.ch = (((((sectr / GEO->nsectors) / GEO->npages ) / GEO->nblocks) / GEO->nplanes) / GEO->nluns) % GEO->nchannels;
			break;

		case NVM_SPEC_VERID_20:
			exp.l.sectr = sectr % GEO->l.nsectr;
			exp.l.chunk = (sectr / GEO->l.nsectr ) % GEO->l.nchunk;
			exp.l.punit = ((sectr / GEO->l.nsectr) / GEO->l.nchunk ) % GEO->l.npunit;
			exp.l.pugrp = (((sectr / GEO->l.nsectr) / GEO->l.nchunk ) / GEO->l.npunit) % GEO->l.npugrp;
			break;
		}

		CU_ASSERT(!nvm_addr_check(exp, DEV));

		switch (func) {
		case 0:
			conv = nvm_addr_gen2dev(DEV, exp);
			act = nvm_addr_dev2gen(DEV, conv);
			break;

		case 1:
			conv = nvm_addr_gen2off(DEV, exp);
			act = nvm_addr_off2gen(DEV, conv);
			break;

		case 2:
			conv = nvm_addr_gen2dev(DEV, exp);
			conv = nvm_addr_dev2off(DEV, conv);
			conv = nvm_addr_off2dev(DEV, conv);
			act = nvm_addr_dev2gen(DEV, conv);
			break;

		default:
			CU_FAIL("Invalid format");
			return;
		}

		CU_ASSERT_EQUAL(act.val, exp.val);
		if ((CU_BRM_VERBOSE == RMODE) && (act.val != exp.val)) {
			printf("Expected: "); nvm_addr_prn(&exp, 1, DEV);
			printf("Got:      "); nvm_addr_prn(&act, 1, DEV);
		}
	}
}

static void conv_chunk_addresses(int func)
{
	size_t tchunk = 0;
	
	switch (nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		tchunk = GEO->g.nchannels * GEO->g.nluns * GEO->g.nplanes *
			GEO->g.nblocks;
		break;
	
	case NVM_SPEC_VERID_20:
		tchunk = GEO->l.npugrp * GEO->l.npunit * GEO->l.nchunk;
		break;

	default:
		CU_FAIL("INVALID VERID");
		return;
	}

	for (size_t chunk = 0; chunk < tchunk; ++chunk) {
		struct nvm_addr exp = { .val = 0 };
		struct nvm_addr act = { .val = 0 };
		uint64_t conv;

		switch (nvm_dev_get_verid(DEV)) {
		case NVM_SPEC_VERID_12:
			exp.g.sec = 0;
			exp.g.pg = 0;
			exp.g.blk = chunk % GEO->nblocks;
			exp.g.pl = (chunk / GEO->nblocks) % GEO->nplanes;
			exp.g.lun = ((chunk / GEO->nblocks) / GEO->nplanes) % GEO->nluns;
			exp.g.ch = (((chunk / GEO->nblocks) / GEO->nplanes) % GEO->nluns) % GEO->nchannels;
			break;

		case NVM_SPEC_VERID_20:
			exp.l.sectr = 0;
			exp.l.chunk = chunk % GEO->l.nchunk;
			exp.l.punit = (chunk / GEO->l.nchunk ) % GEO->l.npunit;
			exp.l.pugrp = ((chunk / GEO->l.nchunk ) / GEO->l.npunit) % GEO->l.npugrp;
			break;
		}

		CU_ASSERT(!nvm_addr_check(exp, DEV));

		switch (func) {
		case 0:
			conv = nvm_addr_gen2lpo(DEV, exp);
			act = nvm_addr_lpo2gen(DEV, conv);
			break;

		default:
			CU_FAIL("Invalid format");
			return;
		}

		CU_ASSERT_EQUAL(act.val, exp.val);
		if ((CU_BRM_VERBOSE == RMODE) && (act.val != exp.val)) {
			printf("Expected: "); nvm_addr_prn(&exp, 1, DEV);
			printf("Got:      "); nvm_addr_prn(&act, 1, DEV);
		}
	}
}

void test_FMT_GEN_DEV_GEN(void)
{
	conv_sectr_addresses(0);	///< gen -> dev -> gen
}

void test_FMT_GEN_OFF_GEN(void)
{
	conv_sectr_addresses(1);	///< gen -> off -> gen
}

void test_FMT_GEN_DEV_OFF_DEV_GEN(void)
{
	conv_sectr_addresses(2);	///< gen -> dev -> off -> dev -> gen
}

void test_FMT_GEN_LPO_GEN(void)
{
	conv_chunk_addresses(0);	///< gen -> lpo -> gen
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_addr_*", argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "fmt gen -> dev -> gen", test_FMT_GEN_DEV_GEN))
		goto out;
	if (!CU_add_test(pSuite, "fmt gen -> off -> gen", test_FMT_GEN_OFF_GEN))
		goto out;
	if (!CU_add_test(pSuite, "fmt gen -> dev -> off -> dev -> gen", test_FMT_GEN_DEV_OFF_DEV_GEN))
		goto out;
	if (!CU_add_test(pSuite, "fmt gen -> lpo -> gen", test_FMT_GEN_LPO_GEN))
		goto out;

	switch(RMODE) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(RMODE);
		CU_basic_run_tests();
		break;
	}

out:
	err = CU_get_error() || \
	      CU_get_number_of_suites_failed() || \
	      CU_get_number_of_tests_failed() || \
	      CU_get_number_of_failures();

	CU_cleanup_registry();

	return err;
}
