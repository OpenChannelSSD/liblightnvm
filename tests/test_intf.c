#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include <CUnit/Automated.h>
#include <CUnit/Basic.h>

#include <liblightnvm.h>
#include <liblightnvm_spec.h>

#define NVM_TEST_RMODE_AUTO 0xA

/*
 * TODO(klaus.jensen): fix hard coded (assumed) values; should be determined
 *                     from device.
 */
#define MDTS 7
#define MPSMIN 4096

#define MDTS_BYTES (MPSMIN * (1 << MDTS))

static int be_id = NVM_BE_ANY;
static int seed = 0;
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

int rmode = CU_BRM_NORMAL;

struct nvm_dev *dev;
const struct nvm_geo *geo;
const struct nvm_nvme_ns *ns;

int MULTIPLE_RESETS_SUPPORTED;

uint32_t WS_MIN;
uint32_t WS_OPT;
uint32_t MW_CUNITS;
uint32_t SECTOR_SIZE;
uint32_t MAX_SCALAR_LBAS;

static int suite_setup(void)
{
	srand(seed);

	dev = nvm_dev_openf(nvm_dev_path, be_id);
	if (!dev) {
		return -1;
	}

	geo = nvm_dev_get_geo(dev);
	ns = nvm_dev_get_ns(dev);

	MULTIPLE_RESETS_SUPPORTED = nvm_dev_get_mccap(dev) & 0x2;

	WS_MIN = nvm_dev_get_ws_min(dev);
	WS_OPT = nvm_dev_get_ws_opt(dev);
	MW_CUNITS = nvm_dev_get_mw_cunits(dev);

	SECTOR_SIZE = geo->l.nbytes;

	MAX_SCALAR_LBAS = (MDTS_BYTES / (WS_MIN * SECTOR_SIZE)) * WS_MIN;

	if (rmode == CU_BRM_VERBOSE) {
		fprintf(stderr, "# nvm_test: multiple resets are %s\n",
			MULTIPLE_RESETS_SUPPORTED ? "supported" : "NOT supported");
	}

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
	case 5:
		be_id = atoi(argv[4]);
		/* FALLTHRU */
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
		/* FALLTHRU */
	case 3:
		seed = atoi(argv[2]);		// Overwrite default seed
		/* FALLTHRU */
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return NULL;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}

	if (rmode == CU_BRM_VERBOSE) {
		printf("# ARGS: {dev: '%s', seed: %u, rmode: 0x%x, be_id: 0x%x}\n",
		       nvm_dev_path, seed, rmode, be_id);
	}

	CU_set_error_action(CUEA_ABORT);

	if (CUE_SUCCESS != CU_initialize_registry())
		return NULL;

	return CU_add_suite(title, suite_setup, suite_teardown);
}
