#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_tgt_name[DISK_NAME_LEN] = "nvm_vblock_tst";

int init_suite1(void)
{
	return 0;
}

int clean_suite1(void)
{
	return 0;
}

void TEST_VBLOCK_GP_N(void)
{
	NVM_TGT tgt;
	NVM_GEO geo;

	NVM_VBLOCK *vblocks;
	int vblocks_total;
	int vblocks_reserved;

	int i;

	tgt = nvm_tgt_open(nvm_tgt_name, 0x0);
	CU_ASSERT_PTR_NOT_NULL(tgt);

	geo = nvm_dev_get_geo(nvm_tgt_get_dev(tgt));

	vblocks_total = geo.nluns * geo.nblocks;	/* Allocate vblocks */
	vblocks = malloc(sizeof(NVM_VBLOCK) * vblocks_total);
	CU_ASSERT_PTR_NOT_NULL(vblocks);

	for (i=0; i < vblocks_total; i++) {
		vblocks[i] = nvm_vblock_new();
		CU_ASSERT_PTR_NOT_NULL(vblocks[i]);
	}

	vblocks_reserved = 0;
	int lun_gets_n[4] = {0, 0, 0, 0};
	int lun_gets_nbad[4] = {0, 0, 0, 0};
	for (i=0; i < vblocks_total; i++) {		/* Reserve vblocks */
		int err, ch, lun, blk;

		blk = i % geo.nblocks;
		ch = i % geo.nchannels;
		lun = i % geo.nluns;
		err = nvm_vblock_gets(vblocks[vblocks_reserved], tgt, ch, lun);
		lun_gets_n[lun]++;
		CU_ASSERT(!err);
		if (err) {
			printf("ch(%02d), lun(%02d), blk(%02d), i(%03d), BAD\n",
				ch, lun, blk, i);
			lun_gets_nbad[lun]++;
			continue;
		}

		printf("ch(%02d), lun(%02d), blk(%02d), i(%03d), GOOD\n",
			ch, lun, blk, i);

		vblocks_reserved++;
	}

	for(i=0; i < geo.nluns; i++) {
		printf("on lun(%d) of #gets(%d) #gets_bad(%d)\n",
			i, lun_gets_n[i], lun_gets_nbad[i]);
	}
	printf("vblocks_reserved(%d)\n", vblocks_reserved);

	for (i=0; i < vblocks_reserved; i++) {		/* Release vblocks */
		int err = nvm_vblock_put(vblocks[i]);
		CU_ASSERT(!err);
		if (err) {
			continue;
		}
	}

	for (i=0; i < vblocks_total; i++) {		/* Deallocate vblocks */
		nvm_vblock_free(&vblocks[i]);
	}
	free(vblocks);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s tgt_name\n", argv[0]);
		return -1;
	}
	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}
	strcpy(nvm_tgt_name, argv[1]);

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_vblock_[get|put] n", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (NULL == CU_add_test(pSuite,
				"nvm_vblock_[get|put] n",
				TEST_VBLOCK_GP_N))
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
