/*
 * raw_tests - Tests for raw interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>

#include "CuTest/CuTest.h"

#define N_TEST_BLOCKS 100

static CuSuite *per_test_suite = NULL;
static char lnvm_dev[DISK_NAME_LEN];
static char lnvm_tgt_type[NVM_TTYPE_NAME_MAX] = "dflash";
static char lnvm_tgt_name[DISK_NAME_LEN] = "liblnvm_test";

static void create_tgt(CuTest *ct)
{
	struct nvm_ioctl_tgt_create c;
	int ret;

	strncpy(c.target.dev, lnvm_dev, DISK_NAME_LEN);
	strncpy(c.target.tgttype, lnvm_tgt_type, NVM_TTYPE_NAME_MAX);
	strncpy(c.target.tgtname, lnvm_tgt_name, DISK_NAME_LEN);
	c.flags = 0;
	c.conf.type = 0;
	c.conf.s.lun_begin = 0;
	c.conf.s.lun_end = 0;

	ret = nvm_create_target(&c);
	CuAssertIntEquals(ct, 0, ret);
}

static void remove_tgt(CuTest *ct)
{
	struct nvm_ioctl_tgt_remove r;
	int ret;

	strncpy(r.tgtname, lnvm_tgt_name, DISK_NAME_LEN);
	r.flags = 0;

	ret = nvm_remove_target(&r);
	CuAssertIntEquals(ct, 0, ret);
}

static long get_nvblocks(NVM_LUN_STAT *stat)
{
	return stat->nr_free_blocks + stat->nr_inuse_blocks +
							stat->nr_bad_blocks;
}

/*
 * Get/Put block without submitting I/Os
 *	- Get block
 *	- Check that a block has moved from free to inuse block list
 *	- Put block
 *	- Check that the block has been returned from inuse to free list
 *	- Repeat 100 times (this does not consume blocks)
 */
static void raw_gp1 (CuTest *ct)
{
	NVM_VBLOCK vblock[N_TEST_BLOCKS];
	NVM_PROV prov = {
		.flags = 0x0,
		.lun_status = NULL,
	};
	NVM_LUN_STAT lun_status = {
		.nr_free_blocks = 0,
		.nr_inuse_blocks = 0,
		.nr_bad_blocks = 0,
	};
	long nvblocks, nvblocks_old;
	int tgt_fd;
	int i;
	int ret = 0;

	create_tgt(ct);
	tgt_fd = nvm_target_open(lnvm_tgt_name, 0x0);
	CuAssertTrue(ct, tgt_fd > 0);

	/* Check LUN status */
	// TODO - wait until we have defined this ioctl

	/* get block from lun 0*/
	vblock[0].vlun_id = 0;

	prov.vblock = &vblock[0];
	prov.lun_status = &lun_status;
	prov.flags |= NVM_PROV_SPEC_LUN | NVM_PROV_LUN_STATE;

	/* get block from lun 0, request LUN satus*/
	ret = nvm_get_block(tgt_fd, 0, &prov);
	CuAssertIntEquals(ct, 0, ret);

	nvblocks = get_nvblocks(&lun_status);

	/* put block, request LUN status*/
	memset(&prov, 0, sizeof(prov));

	prov.vblock = &vblock[0];
	prov.lun_status = &lun_status;
	prov.flags |= NVM_PROV_SPEC_LUN | NVM_PROV_LUN_STATE;

	ret = nvm_put_block(tgt_fd, &prov);

	nvblocks_old = nvblocks;
	nvblocks = get_nvblocks(&lun_status);
	CuAssertIntEquals(ct, nvblocks_old, nvblocks);

	nvm_target_close(tgt_fd);
	remove_tgt(ct);
}

static void init_lib(CuTest *ct)
{
	int ret = nvm_init();
	CuAssertIntEquals(ct, 0, ret);
}

static void exit_lib(CuTest *ct)
{
	nvm_exit();
}

CuSuite *raw_GetSuite()
{
	per_test_suite = CuSuiteNew();

	SUITE_ADD_TEST(per_test_suite, init_lib);
	SUITE_ADD_TEST(per_test_suite, raw_gp1);
	SUITE_ADD_TEST(per_test_suite, exit_lib);
}

void run_all_test(void)
{
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();

	CuSuiteAddSuite(suite, (CuSuite*) raw_GetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	CuStringDelete(output);
	CuSuiteDelete(suite);

	free(per_test_suite);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s lnvm_dev / lnvm_dev: LightNVM device\n",
									argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("Argument lnvm_dev can be maximum %d characters\n",
							DISK_NAME_LEN - 1);
	}

	strcpy(lnvm_dev, argv[1]);

	run_all_test();

	return 0;
}
