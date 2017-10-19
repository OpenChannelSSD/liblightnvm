#include "test_intf.c"

static void conv(int func)
{
	size_t tsectr = 0;
	
	switch (nvm_dev_get_verid(dev)) {
	case NVM_SPEC_VERID_12:
		tsectr = geo->g.nchannels * geo->g.nluns * geo->g.nplanes *
			geo->g.nblocks * geo->g.npages * geo->g.nsectors;
		break;
	
	case NVM_SPEC_VERID_20:
		tsectr = geo->l.npugrp * geo->l.npunit * geo->l.nchunk *
			 geo->l.nsectr;
		break;

	default:
		CU_FAIL("INVALID VERID");
		return;
	}

	for (size_t sectr = 0; sectr < tsectr; ++sectr) {
		struct nvm_addr exp = { .val = 0 };
		struct nvm_addr act = { .val = 0 };
		uint64_t conv;

		switch (nvm_dev_get_verid(dev)) {
		case NVM_SPEC_VERID_12:
			exp.g.sec = sectr % geo->nsectors;
			exp.g.pg = (sectr / geo->nsectors ) % geo->npages;
			exp.g.blk = ((sectr / geo->nsectors) / geo->npages ) % geo->nblocks;
			exp.g.pl = (((sectr / geo->nsectors) / geo->npages ) / geo->nblocks) % geo->nplanes;
			exp.g.lun = ((((sectr / geo->nsectors) / geo->npages ) / geo->nblocks) / geo->nplanes) % geo->nluns;
			exp.g.ch = (((((sectr / geo->nsectors) / geo->npages ) / geo->nblocks) / geo->nplanes) / geo->nluns) % geo->nchannels;
			break;

		case NVM_SPEC_VERID_20:
			exp.l.sectr = sectr % geo->l.nsectr;
			exp.l.chunk = (sectr / geo->l.nsectr ) % geo->l.nchunk;
			exp.l.punit = ((sectr / geo->l.nsectr) / geo->l.nchunk ) % geo->l.npunit;
			exp.l.pugrp = (((sectr / geo->l.nsectr) / geo->l.nchunk ) / geo->l.npunit) % geo->l.npugrp;
			break;
		}

		CU_ASSERT(!nvm_addr_check(exp, dev));

		switch (func) {
		case 0:
			conv = nvm_addr_gen2dev(dev, exp);
			act = nvm_addr_dev2gen(dev, conv);
			break;

		case 1:
			conv = nvm_addr_gen2off(dev, exp);
			act = nvm_addr_off2gen(dev, conv);
			break;

		case 2:
			conv = nvm_addr_gen2lba(dev, exp);
			act = nvm_addr_lba2gen(dev, conv);
			break;

		case 3:
			conv = nvm_addr_gen2dev(dev, exp);
			conv = nvm_addr_dev2lba(dev, conv);
			act = nvm_addr_lba2gen(dev, conv);
			break;

		case 4:
			conv = nvm_addr_gen2dev(dev, exp);
			conv = nvm_addr_dev2off(dev, conv);
			act = nvm_addr_off2gen(dev, conv);
			break;

		default:
			CU_FAIL("Invalid format");
			return;
		}

		CU_ASSERT_EQUAL(act.val, exp.val);
		if ((CU_BRM_VERBOSE == rmode) && (act.val != exp.val)) {
			printf("Expected: "); nvm_addr_prn(&exp, 1, dev);
			printf("Got:      "); nvm_addr_prn(&act, 1, dev);
		}
	}
}

void test_FMT_GEN_DEV(void)
{
	conv(0);	///< gen -> dev -> gen
}

void test_FMT_GEN_OFF(void)
{
	conv(1);	///< gen -> off -> gen
}

void test_FMT_GEN_LBA(void)
{
	conv(2);	///< gen -> LBA -> gen
}

void test_FMT_GEN_DEV_LBA(void)
{
	conv(3);	///< gen -> dev -> LBA -> gen
}

void test_FMT_GEN_DEV_OFF(void)
{
	conv(4);	///< gen -> dev -> off -> gen
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_addr_*", argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "fmt gen -> dev -> gen", test_FMT_GEN_DEV))
		goto out;
	if (!CU_add_test(pSuite, "fmt gen -> lba -> gen", test_FMT_GEN_LBA))
		goto out;
	if (!CU_add_test(pSuite, "fmt gen -> off -> gen", test_FMT_GEN_OFF))
		goto out;
	if (!CU_add_test(pSuite, "fmt gen -> dev -> lba -> gen", test_FMT_GEN_DEV_LBA))
		goto out;
	if (!CU_add_test(pSuite, "fmt gen -> dev -> off -> gen", test_FMT_GEN_DEV_OFF))
		goto out;

	switch(rmode) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(rmode);
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
