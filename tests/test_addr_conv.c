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
static struct nvm_addr addr;

size_t compare_buffers(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	for (int i = 0; i < nbytes; ++i) {
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
	geo = nvm_dev_attr_geo(dev);

	// Construct an arbitrary address somewhat in the middle of the geometry
	addr.ppa = 0;
	addr.g.ch = (geo->nchannels-1) / 2;
	addr.g.lun = (geo->nluns-1) / 2;
	addr.g.pl = (geo->nplanes-1) / 2;
	addr.g.blk = (geo->nblocks-1) / 2;
	addr.g.pg = (geo->npages-1) / 2;
	addr.g.sec = (geo->nsectors-1) / 2;

	return 0;
}

int teardown(void)
{
	geo = NULL;
	nvm_dev_close(dev);

	return 0;
}

void test_FMT_GEN_DEV(void)
{
	uint64_t conv;
	struct nvm_addr actual;
	
	conv = nvm_addr_gen2dev(dev, addr);
	actual = nvm_addr_dev2gen(dev, conv);

	CU_ASSERT_EQUAL(actual.ppa, addr.ppa);
	if (actual.ppa != addr.ppa) {
		printf("Expected: "); nvm_addr_pr(addr);
		printf("Got:      "); nvm_addr_pr(actual);
	}
}

void test_FMT_GEN_OFF(void)
{
	uint64_t conv;
	struct nvm_addr actual;
	
	conv = nvm_addr_gen2off(dev, addr);
	actual = nvm_addr_off2gen(dev, conv);

	CU_ASSERT_EQUAL(actual.ppa, addr.ppa);
	if (actual.ppa != addr.ppa) {
		printf("Expected: "); nvm_addr_pr(addr);
		printf("Got:      "); nvm_addr_pr(actual);
	}
}

void test_FMT_GEN_LBA(void)
{
	uint64_t conv;
	struct nvm_addr actual;
	
	conv = nvm_addr_gen2lba(dev, addr);
	actual = nvm_addr_lba2gen(dev, conv);

	CU_ASSERT_EQUAL(actual.ppa, addr.ppa);
	if (actual.ppa != addr.ppa) {
		printf("Expected: "); nvm_addr_pr(addr);
		printf("Got:      "); nvm_addr_pr(actual);
	}
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
