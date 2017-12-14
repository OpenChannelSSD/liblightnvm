#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <liblightnvm.h>
#include <CUnit/Basic.h>

static int rmode = CU_BRM_VERBOSE;
static int seed = 0;
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
	}
	geo = nvm_dev_get_geo(dev);

	srand(seed);

	return 0;
}

int teardown(void)
{
	geo = NULL;
	nvm_dev_close(dev);

	return 0;
}

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

/**
 * Tests: gen -> dev -> gen
 */
void test_FMT_GEN_DEV(void)
{
	conv(0);
}

/**
 * Tests: gen -> off -> gen
 */
void test_FMT_GEN_OFF(void)
{
	conv(1);
}

/**
 * Tests: gen -> LBA -> gen
 */
void test_FMT_GEN_LBA(void)
{
	conv(2);
}

/**
 * Tests: gen -> dev -> LBA -> gen
 */
void test_FMT_GEN_DEV_LBA(void)
{
	conv(3);
}

/**
 * Tests: gen -> dev -> off -> gen
 */
void test_FMT_GEN_DEV_OFF(void)
{
	conv(4);
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 4:
		switch(atoi(argv[3])) {
		case 2:
			rmode = CU_BRM_VERBOSE;
			break;
		case 1:
			rmode = CU_BRM_SILENT;
			break;
		case 0:
		default:
			rmode = CU_BRM_NORMAL;
			break;
		}
	case 3:
		seed = atoi(argv[2]);
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return 1;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}

	if (!seed) {	// Default arbitrary seed
		seed = time(NULL);
	}

	printf("# TEST_INPUT: {dev: '%s', seed: %u, rmode: 0x%x}\n",
	       nvm_dev_path, seed, rmode);

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_addr_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "fmt gen -> dev -> gen", test_FMT_GEN_DEV)) ||
	(NULL == CU_add_test(pSuite, "fmt gen -> lba -> gen", test_FMT_GEN_LBA)) ||
	(NULL == CU_add_test(pSuite, "fmt gen -> off -> gen", test_FMT_GEN_OFF)) ||
	(NULL == CU_add_test(pSuite, "fmt gen -> dev -> lba -> gen", test_FMT_GEN_DEV_LBA)) ||
	(NULL == CU_add_test(pSuite, "fmt gen -> dev -> off -> gen", test_FMT_GEN_DEV_OFF)) ||
	0)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(rmode);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
