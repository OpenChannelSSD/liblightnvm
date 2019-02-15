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

static int BE_ID = NVM_BE_ANY;
static int SEED = 0;
static char NVM_DEV_PATH[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

int RMODE = CU_BRM_NORMAL;

struct nvm_dev *DEV;
const struct nvm_geo *GEO;
const struct nvm_nvme_ns *NS;

int MULTIPLE_RESETS_SUPPORTED;

uint32_t WS_MIN;
uint32_t WS_OPT;
uint32_t MW_CUNITS;
uint32_t NSECTR;
uint32_t SECTOR_SIZE;
uint32_t OOB_SIZE;
uint32_t MAX_SCALAR_LBAS;

static int suite_setup(void)
{
	srand(SEED);

	DEV = nvm_dev_openf(NVM_DEV_PATH, BE_ID);
	if (!DEV) {
		return -1;
	}

	GEO = nvm_dev_get_geo(DEV);
	NS = nvm_dev_get_ns(DEV);

	MULTIPLE_RESETS_SUPPORTED = nvm_dev_get_mccap(DEV) & 0x2;

	WS_MIN = nvm_dev_get_ws_min(DEV);
	WS_OPT = nvm_dev_get_ws_opt(DEV);
	MW_CUNITS = nvm_dev_get_mw_cunits(DEV);

	NSECTR = GEO->l.nsectr;
	SECTOR_SIZE = GEO->l.nbytes;
	OOB_SIZE = GEO->l.nbytes_oob;

	MAX_SCALAR_LBAS = (MDTS_BYTES / (WS_MIN * SECTOR_SIZE)) * WS_MIN;

	if (RMODE == CU_BRM_VERBOSE) {
		fprintf(stderr, "# nvm_test: multiple resets are %s\n",
			MULTIPLE_RESETS_SUPPORTED ? "supported" : "NOT supported");
	}

	return 0;
}

static int suite_teardown(void)
{
	GEO = NULL;
	nvm_dev_close(DEV);

	return 0;
}

static int _cli_parse(int argc, char *argv[])
{
	SEED = time(NULL);			// Default arbitrary SEED

	switch(argc) {
	case 5:
		BE_ID = atoi(argv[4]);
		/* FALLTHRU */
	case 4:
		switch(atoi(argv[3])) {
		case NVM_TEST_RMODE_AUTO:
			RMODE = NVM_TEST_RMODE_AUTO;
			break;
		case 2:
			RMODE = CU_BRM_VERBOSE;
			break;
		case 1:
			RMODE = CU_BRM_SILENT;
			break;
		case 0:
		default:
			RMODE = CU_BRM_NORMAL;
			break;
		}
		/* FALLTHRU */
	case 3:
		SEED = atoi(argv[2]);		// Overwrite default SEED
		/* FALLTHRU */
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return -1;
                }
		strncpy(NVM_DEV_PATH, argv[1], NVM_DEV_PATH_LEN);
		break;
	}

	if (RMODE == CU_BRM_VERBOSE) {
		printf("# ARGS: {DEV: '%s' SEED: %u RMODE: 0x%x be_id: 0x%x}\n",
		       NVM_DEV_PATH, SEED, RMODE, BE_ID);
	}

	return 0;
}

static CU_pSuite suite_create(const char *title, int argc, char *argv[],
			      int skip)
{
	if (_cli_parse(argc, argv)) {
		return NULL;
	}
	CU_set_error_action(CUEA_ABORT);

	if (CUE_SUCCESS != CU_initialize_registry())
		return NULL;

	if (skip) {
		return CU_add_suite(title, NULL, NULL);
	}

	return CU_add_suite(title, suite_setup, suite_teardown);
}
