#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

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
	size_t tsectr = geo->l.npugrp * geo->l.npunit * geo->l.nchunk *
			 geo->l.nsectr;

	for (size_t sectr = 0; sectr < tsectr; ++sectr) {
		struct nvm_addr exp = { .val = 0 };
		struct nvm_addr act = { .val = 0 };
		uint64_t conv;

		exp.l.sectr = sectr % geo->l.nsectr;
		exp.l.chunk = (sectr / geo->l.nsectr ) % geo->l.nchunk;
		exp.l.punit = ((sectr / geo->l.nsectr) / geo->l.nchunk ) % geo->l.npunit;
		exp.l.pugrp = (((sectr / geo->l.nsectr) / geo->l.nchunk ) / geo->l.npunit) % geo->l.npugrp;

		CU_ASSERT(!nvm_addr_check(exp, dev))

		switch (func) {
		case 0:
			conv = nvm_addr_gen2dev(dev, exp);
			act = nvm_addr_dev2gen(dev, conv);
			break;

		default:
			CU_FAIL("Invalid format");
			return;
		}

		CU_ASSERT_EQUAL(act.val, exp.val);
		if (act.val != exp.val) {
			printf("Expected: "); nvm_addr_pr(exp);
			printf("Got:      "); nvm_addr_pr(act);
		}
	}
}

void test_GEN2DEV2GEN(void)
{
	conv(0);
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return 1;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_addr_{gen2dev,dev2gen}", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "fmt gen -> dev -> gen", test_GEN2DEV2GEN)) ||
	0)
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_NORMAL);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
