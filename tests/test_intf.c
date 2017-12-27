#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#include <liblightnvm.h>
#include <liblightnvm_spec.h>

#define NVM_TEST_RMODE_AUTO 0xA

static int rmode = CU_BRM_NORMAL;
static int seed = 0;
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";
static struct nvm_dev *dev;
static const struct nvm_geo *geo;

static int suite_setup(void)
{
	srand(seed);

	dev = nvm_dev_open(nvm_dev_path);
	if (!dev)
		return -1;

	geo = nvm_dev_get_geo(dev);

	return 0;
}

static int suite_teardown(void)
{
	geo = NULL;
	nvm_dev_close(dev);

	return 0;
}

static CU_pSuite suite_create(const char *title, int argc, char *argv[])
{
	seed = time(NULL);			// Default arbitrary seed

	switch(argc) {
	case 4:
		switch(atoi(argv[3])) {
		case NVM_TEST_RMODE_AUTO:
			rmode = NVM_TEST_RMODE_AUTO;
			break;
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
		seed = atoi(argv[2]);		// Overwrite default seed
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return NULL;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}

	printf("# TEST_INPUT: {dev: '%s', seed: %u, rmode: 0x%x}\n",
	       nvm_dev_path, seed, rmode);

	if (CUE_SUCCESS != CU_initialize_registry())
		return NULL;

	return CU_add_suite(title, suite_setup, suite_teardown);
}

