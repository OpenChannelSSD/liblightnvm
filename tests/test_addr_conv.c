#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

// Parsed from CLI
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

// Managed by setup/teardown and used by tests
static struct nvm_dev *dev;
static const struct nvm_geo *geo;

size_t compare_buffers(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	for (size_t i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			++diff;
		}
	}

	return diff;
}

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

void _test_FMT_CONV(int func)
{
	size_t tsecs = geo->nchannels * geo->nluns * geo->nplanes *
		       geo->nblocks * geo->npages * geo->nsectors;

	for (size_t sec = 0; sec < tsecs; ++sec) {
		struct nvm_addr expected;
		struct nvm_addr actual;
		uint64_t conv;
		int res;

		expected.ppa = 0;
		expected.g.sec = sec % geo->nsectors;
		expected.g.pg = (sec / geo->nsectors ) % geo->npages;
		expected.g.blk = ((sec / geo->nsectors) / geo->npages ) % geo->nblocks;
		expected.g.pl = (((sec / geo->nsectors) / geo->npages ) / geo->nblocks) % geo->nplanes;
		expected.g.lun = ((((sec / geo->nsectors) / geo->npages ) / geo->nblocks) / geo->nplanes) % geo->nluns;
		expected.g.ch = (((((sec / geo->nsectors) / geo->npages ) / geo->nblocks) / geo->nplanes) / geo->nluns) % geo->nchannels;

		res = nvm_addr_check(expected, dev);
		CU_ASSERT(!res)

		switch (func) {
		case 0:
			conv = nvm_addr_gen2dev(dev, expected);
			actual = nvm_addr_dev2gen(dev, conv);
			break;

		case 1:
			conv = nvm_addr_gen2off(dev, expected);
			actual = nvm_addr_off2gen(dev, conv);
			break;

		case 2:
			conv = nvm_addr_gen2lba(dev, expected);
			actual = nvm_addr_lba2gen(dev, conv);
			break;

		case 3:
			conv = nvm_addr_gen2dev(dev, expected);
			conv = nvm_addr_dev2lba(dev, conv);
			actual = nvm_addr_lba2gen(dev, conv);
			break;

		case 4:
			conv = nvm_addr_gen2dev(dev, expected);
			conv = nvm_addr_dev2off(dev, conv);
			actual = nvm_addr_off2gen(dev, conv);
			break;

		default:
			CU_FAIL("Invalid format");
			return;
		}

		CU_ASSERT_EQUAL(actual.ppa, expected.ppa);
		if (actual.ppa != expected.ppa) {
			printf("Expected: "); nvm_addr_pr(expected);
			printf("Got:      "); nvm_addr_pr(actual);
		}
	}
}

/**
 * Tests: gen <-> dev
 */
void test_FMT_GEN_DEV(void)
{
	_test_FMT_CONV(0);
}

/**
 * Tests: gen <-> off
 */
void test_FMT_GEN_OFF(void)
{
	_test_FMT_CONV(1);
}

/**
 * Tests: gen <-> LBA
 */
void test_FMT_GEN_LBA(void)
{
	_test_FMT_CONV(2);
}

/**
 * Tests: gen <-> dev <-> LBA
 */
void test_FMT_GEN_DEV_LBA(void)
{
	_test_FMT_CONV(3);
}

/**
 * Tests: gen <-> dev <-> off
 */
void test_FMT_GEN_DEV_OFF(void)
{
	_test_FMT_CONV(4);
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

	pSuite = CU_add_suite("nvm_addr_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "fmt gen <-> dev", test_FMT_GEN_DEV)) ||
	(NULL == CU_add_test(pSuite, "fmt gen <-> lba", test_FMT_GEN_LBA)) ||
	(NULL == CU_add_test(pSuite, "fmt gen <-> off", test_FMT_GEN_OFF)) ||
	(NULL == CU_add_test(pSuite, "fmt gen -> dev -> lba -> gen", test_FMT_GEN_DEV_LBA)) ||
	(NULL == CU_add_test(pSuite, "fmt gen -> dev -> off -> gen", test_FMT_GEN_DEV_OFF)) ||
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
