#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int channel = 0;
static int lun = 0;

static struct nvm_dev *dev;
static const struct nvm_geo *geo;
static struct nvm_addr lun_addr;

int setup(void)
{
	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		CU_ASSERT_PTR_NOT_NULL(dev);
	}
	geo = nvm_dev_attr_geo(dev);

	lun_addr.ppa = 0;
	lun_addr.g.ch = channel;
	lun_addr.g.lun = lun;

	return 0;
}

int teardown(void)
{
	nvm_dev_close(dev);

	return 0;
}

// Get BBT, change it, then persist it using nvm_bbt_set and verify it
// using nvm_bbt_get
void test_BBT_GET_SET_GET(void)
{
	return;
}

// Mark some blocks using nvm_bbt_mark, then use get to verify that the
// change persisted
void test_BBT_MARK_GET(void)
{
	return;
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 4:
		lun = atoi(argv[3]);
	case 3:
		channel = atoi(argv[2]);
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
	(NULL == CU_add_test(pSuite, "BBT get -> set -> get", test_BBT_GET_SET_GET)) ||
	(NULL == CU_add_test(pSuite, "BBT mark -> get", test_BBT_MARK_GET)) ||
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
