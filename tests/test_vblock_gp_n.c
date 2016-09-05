/*
 * This test attempts to allocate all blocks on the given device.
 * Allocating all blocks will fail at least for two reasons:
 *
 * - vblocks might be reserved by others
 * - vblocks might be bad
 *
 * The test therefore only fails when the amount of successfully allocated
 * vblocks is below threshold TOTAL_NUMBER_OF_VBLOCK - k. Where k is some
 * arbitrarily chosen number defaulting to 8 failed gets.
 *
 * NOTE: Be wary about running this test on actual hardware since it might
 *	 wear out an MLC device after about 1000-3000 runs. This is currently
 *	 true since 'vblock_get' erases a block when responding to an _GET.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev_name[DISK_NAME_LEN] = "nvm_vblock_tst";
int k = 10;	// Total number of nvm_vblock_gets allowed to fail

void TEST_VBLOCK_GP_N(void)
{
	NVM_DEV dev;
	NVM_GEO geo;

	NVM_VBLOCK *vblocks;	/* Array of vblocks */
	int vblocks_total;	/* Number of vblocks on device / allocated */
	int vblocks_reserved;	/* Number of vblocks successfully reserved */

	int ngets;		/* Total number of ngets */
	int ngets_failed;	/* Total number of failed ngets */
	int *ngets_lun;		/* Number of gets per lun */
	int *ngets_lun_failed;	/* Number of failed gets per lun */

	int i;

	dev = nvm_dev_open(nvm_dev_name);
	CU_ASSERT_PTR_NOT_NULL(dev);

	geo = nvm_dev_get_geo(dev);

	ngets = 0;
	ngets_lun = malloc(sizeof(ngets_lun)*geo.nluns);
	memset(ngets_lun, 0, sizeof(ngets_lun)*geo.nluns);

	ngets_failed = 0;
	ngets_lun_failed = malloc(sizeof(ngets_lun_failed)*geo.nluns);
	memset(ngets_lun_failed, 0, sizeof(ngets_lun_failed)*geo.nluns);

	vblocks_total = geo.nluns * geo.nblocks;	/* Allocate vblocks */
	vblocks = malloc(sizeof(NVM_VBLOCK) * vblocks_total);
	CU_ASSERT_PTR_NOT_NULL(vblocks);

	for (i=0; i < vblocks_total; i++) {
		vblocks[i] = nvm_vblock_new();
		CU_ASSERT_PTR_NOT_NULL(vblocks[i]);
	}

	vblocks_reserved = 0;
	for (i=0; i < vblocks_total; i++) {		/* Reserve vblocks */
		int err, ch, lun;

		ch = i % geo.nchannels;
		lun = i % geo.nluns;
		err = nvm_vblock_gets(vblocks[vblocks_reserved], dev, ch, lun);
		ngets++;
		ngets_lun[lun]++;
		if (err) {
			ngets_failed++;
			ngets_lun_failed[lun]++;
			continue;
		}

		vblocks_reserved++;
	}

	/* Check that we did as much as we expected */
	CU_ASSERT(ngets == vblocks_total);

	/* Check that we got a sufficient amount of vblocks */
	CU_ASSERT(vblocks_total - vblocks_reserved < k)

	/* That is... no more than k failures */
	CU_ASSERT(ngets_failed <= k);

	/* Print counters / totals
	printf("vblocks_total(%d)\n", vblocks_total);
	printf("vblocks_reserved(%d)\n", vblocks_reserved);
	printf("ngets(%d), ngets_failed(%d)\n", ngets, ngets_failed);
	for(i=0; i < geo.nluns; i++) {
		printf("i(%d), ngets_lun(%d) / ngets_lun_failed(%d)\n",
			i, ngets_lun[i], ngets_lun_failed[i]);
	}
	*/

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
		printf("Usage: %s dev_name\n", argv[0]);
		return -1;
	}
	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}
	strcpy(nvm_dev_name, argv[1]);

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("_vblock_[gets|put] n", NULL, NULL);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (NULL == CU_add_test(pSuite, "_vblock_[gets|put] n", TEST_VBLOCK_GP_N))
	{
		CU_cleanup_registry();
		return CU_get_error();
	}

	CU_basic_set_mode(CU_BRM_SILENT);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}
