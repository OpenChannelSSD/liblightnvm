#include "test_intf.c"

#define FLUSH_ALL 1

// States used when verifying that bad-block-table can be set
static enum nvm_bbt_state states[] = {
	NVM_BBT_FREE,
	NVM_BBT_HMRK
};
static int nstates = sizeof(states) / sizeof(states[0]);

static struct nvm_addr arb_lun_addr(void)
{
	struct nvm_addr addr = { .val = 0 };
	int arb = rand();

	addr.g.ch = arb % geo->nchannels;
	addr.g.lun = arb % geo->nluns;

	return addr;
}

static void verify_counters(struct nvm_dev *dev, const struct nvm_bbt *bbt)
{
	uint32_t nbad = 0, ngbad = 0, ndmrk = 0, nhmrk = 0;

	for (uint64_t i = 0; i < bbt->nblks; ++i) {
		switch (bbt->blks[i]) {
		case NVM_BBT_FREE:
			break;
		case NVM_BBT_BAD:
			++nbad;
			break;
		case NVM_BBT_GBAD:
			++ngbad;
			break;
		case NVM_BBT_DMRK:
			++ndmrk;
			break;
		case NVM_BBT_HMRK:
			++nhmrk;
			break;

		default:
			CU_FAIL("Invalid blk-state");
			break;
		}
	}

	/* b0rk3d
	if (nvm_dev_get_verid(dev) == 0x2) {	// Spec 2.0
		nbad = nbad / geo->nplanes;
		ngbad = ngbad / geo->nplanes;
		ndmrk = ndmrk / geo->nplanes;
		nhmrk = nhmrk / geo->nplanes;
	}
	*/

	CU_ASSERT_EQUAL(bbt->nbad, nbad);
	CU_ASSERT_EQUAL(bbt->ngbad, ngbad);
	CU_ASSERT_EQUAL(bbt->ndmrk, ndmrk);
	CU_ASSERT_EQUAL(bbt->nhmrk, nhmrk);
}

/**
 * Test that we can get a valid bad-block-table from a device
 */
static void bbt_get(int bbts_cached)
{
	struct nvm_addr lun_addr = arb_lun_addr();
	struct nvm_ret ret = {0, 0};
	const struct nvm_bbt *bbt;

	nvm_dev_set_bbts_cached(dev, bbts_cached);
	if (FLUSH_ALL && nvm_bbt_flush_all(dev, &ret)) {
		CU_FAIL("FAILED: nvm_bbt_flush_all");
		return;
	}

	bbt = nvm_bbt_get(dev, lun_addr, &ret);	// Verify that we can call it
	CU_ASSERT_PTR_NOT_NULL(bbt);
	if (!bbt)
		return;

	// Verify that it contains the expected number of blocks
	CU_ASSERT_EQUAL(bbt->nblks, geo->nplanes * geo->nblocks);
	if (bbt->nblks != geo->nplanes * geo->nblocks) {
		CU_FAIL("FAILED: Unexpected value of bbt->nblks");
		return;
	}

	// Verify that it contains valid states
	for (uint64_t i = 0; i < bbt->nblks; ++i) {
		switch(bbt->blks[i]) {
			case NVM_BBT_FREE:
			case NVM_BBT_BAD:
			case NVM_BBT_GBAD:
			case NVM_BBT_HMRK:
			case NVM_BBT_DMRK:
				break;
			default:
				CU_FAIL("FAILED: Invalid value bbt->blks[i]");
				return;
		}
	}

	verify_counters(dev, bbt);

	return;
}

void test_BBT_GET(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}

	bbt_get(0);
}

void test_BBT_GET_CACHED(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}

	bbt_get(1);
}

//
// Test that we can set bbt using `nvm_bbt_mark`
//
// @note
// The test changes the state of NVM_NADDR_MAX number of blocks nstates times.
// Verified is done by retrieving the bbt and comparing actual state with
// expected state.
//
// @warn
// This will alter the state of the bad-block-table on the device. It will most
// likely leave the bad-block-table in a different state than it was in before
// running this test
//
static void bbt_mark_naddr(unsigned int naddr_pr_call, int bbts_cached)
{
	struct nvm_addr lun_addr = arb_lun_addr();
	struct nvm_ret ret = {0,0};
	const int nblks = NVM_NADDR_MAX;
	
	const int nvblks = nblks / geo->nplanes;
	const int vblk_ofz = 4;
	const int vblk_skip = (geo->nblocks - vblk_ofz) / nvblks;

	struct nvm_addr addrs[NVM_NADDR_MAX];

	nvm_dev_set_bbts_cached(dev, bbts_cached);
	if (FLUSH_ALL && nvm_bbt_flush_all(dev, &ret)) {
		CU_FAIL("FAILED: nvm_bbt_flush_all");
		return;
	}

	for (int i = 0; i < nblks; i += geo->nplanes) {	// Construct addresses
		int vblk = vblk_ofz + (i / geo->nplanes) * vblk_skip;
		for (size_t pl = 0; pl < geo->nplanes; ++pl) {
			addrs[i + pl].ppa = lun_addr.ppa;
			addrs[i + pl].g.blk = vblk;
			addrs[i + pl].g.pl = pl;
		}
	}
	
	for (int i = 0; i < nblks; ++i) {	// Verify constructed addrs
		if (nvm_addr_check(addrs[i], dev)) {
			CU_FAIL("FAILED: Constructing addresses");
			return;
		}
	}

	for (int st_i = 0; st_i < nstates; ++st_i) {
		const struct nvm_bbt *bbt;
		const enum nvm_bbt_state state = states[st_i];

		// Persist the results using "NVM_NADDR_MAX / naddr_pr_call"
		// number of calls to nvm_bbt_mark
		for (int ofz = 0; ofz < nblks; ofz += naddr_pr_call) {
			int err;
			
			err = nvm_bbt_mark(dev, &addrs[ofz], naddr_pr_call,
					   state, &ret);
			if (err) {
				CU_FAIL("FAILED: nvm_bbt_mark");
				return;
			}
		}

		bbt = nvm_bbt_get(dev, lun_addr, &ret);
		CU_ASSERT_PTR_NOT_NULL(bbt);
		if (!bbt) {
			CU_FAIL("FAILED: Retrieving bbt for verification");
			return;
		}

		for (int i = 0; i < nblks; ++i) {	// Verify state
			int idx = addrs[i].g.blk * geo->nplanes + addrs[i].g.pl;
			CU_ASSERT_EQUAL(bbt->blks[idx], state);
			if (bbt->blks[idx] != state) {
				CU_FAIL("FAILED: Unexpected value of bbt");
				break;
			}
		}
	}
}

void test_BBT_MARK_NADDR_MAX(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}

	bbt_mark_naddr(NVM_NADDR_MAX, 0);
}

void test_BBT_MARK_NADDR_MAX2(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}

	bbt_mark_naddr(NVM_NADDR_MAX / 2, 0);
}

void test_BBT_MARK_NADDR_MAX4(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}

	bbt_mark_naddr(NVM_NADDR_MAX / 4, 0);
}

void test_BBT_MARK_NADDR_1(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
	}

	bbt_mark_naddr(1, 0);
}

void test_BBT_MARK_NADDR_MAX_CACHED(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}
	bbt_mark_naddr(NVM_NADDR_MAX, 1);
}

void test_BBT_MARK_NADDR_MAX2_CACHED(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}
	bbt_mark_naddr(NVM_NADDR_MAX / 2, 1);
}

void test_BBT_MARK_NADDR_MAX4_CACHED(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}
	bbt_mark_naddr(NVM_NADDR_MAX / 4, 1);
}

void test_BBT_MARK_NADDR_1_CACHED(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}
	bbt_mark_naddr(1, 1);
}

// Test that we can set bbt using `nvm_bbt_set`
//
// @warn
// This will alter the state of the bad-block-table on the device.
// It will most likely leave the bad-block-table in a different state than
// it was in before running this test
//
static void bbt_set(int bbts_cached)
{
	struct nvm_addr lun_addr = arb_lun_addr();
	struct nvm_ret ret = {0,0};
	struct nvm_bbt *bbt_exp;
	const struct nvm_bbt *bbt_act;
	int res;
	
	nvm_dev_set_bbts_cached(dev, bbts_cached);
	if (FLUSH_ALL && nvm_bbt_flush_all(dev, &ret)) {
		CU_FAIL("FAILED: nvm_bbt_flush_all");
		return;
	}

	bbt_exp = nvm_bbt_alloc_cp(nvm_bbt_get(dev, lun_addr, &ret));
	if (!bbt_exp) {
		CU_FAIL("FAILED: nvm_bbt_get -- prior to nvm_bbt_set");
		return;
	}

	// Check that we have the expected number of blocks
	CU_ASSERT_EQUAL(bbt_exp->nblks, geo->nplanes * geo->nblocks);

	// Flip the state four plane-spanning-blocks
	int nblks = 4 * geo->nplanes;
	for (int i = 0; i < nblks; i += geo->nplanes) {
		int plane_blk = (10 * geo->nplanes) + i * 4;
		for (size_t pl = 0; pl < geo->nplanes; ++pl) {
			int idx = plane_blk + pl;

			if (bbt_exp->blks[idx])
				bbt_exp->blks[idx] = NVM_BBT_FREE;
			else
				bbt_exp->blks[idx] = NVM_BBT_HMRK;

			if (CU_BRM_VERBOSE == rmode)
				printf("FLIPPED: blk(%d), idx(%d), exp(%d)\n",
					plane_blk, idx, bbt_exp->blks[idx]);
		}
	}

	res = nvm_bbt_set(dev, bbt_exp, &ret);	// Persist changes
	if (res < 0) {
		CU_FAIL("FAILED: nvm_bbt_set");
		nvm_bbt_free(bbt_exp);
		return;
	}

	bbt_act = nvm_bbt_get(dev, lun_addr, &ret);
	if (!bbt_act) {
		CU_FAIL("FAILED: nvm_bbt_get -- after nvm_bbt_set")
		nvm_bbt_free(bbt_exp);
		return;
	}

	CU_ASSERT_EQUAL(bbt_exp->nblks, bbt_act->nblks);	// Verify bbt

	res = 0;
	for (uint64_t blk = 0; blk < bbt_act->nblks; ++blk) {
		CU_ASSERT_EQUAL(bbt_exp->blks[blk], bbt_act->blks[blk]);
		if (bbt_exp->blks[blk] != bbt_act->blks[blk]) {
			CU_FAIL("FAILED: Unexpected value of bbt");
			res = -1;
			break;
		}
	}

	if ((CU_BRM_VERBOSE == rmode) && res) {
		for (uint64_t blk = 0; blk < bbt_act->nblks; ++blk)
			if (bbt_exp->blks[blk] != bbt_act->blks[blk])
				printf("FAILED: blk(%lu): exp(%d) != act(%d)\n",
					blk, bbt_exp->blks[blk], bbt_act->blks[blk]);
	}

	nvm_bbt_free(bbt_exp);

	return;
}

void test_BBT_SET(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}

	bbt_set(0);
}

void test_BBT_SET_CACHED(void)
{
	if (NVM_SPEC_VERID_12 != nvm_dev_get_verid(dev)) {
		CU_FAIL("FAILED: device is spec. 1.2");
		return;
	}

	bbt_set(1);
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_bbt_*", argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "nvm_bbt_get", test_BBT_GET))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX)", test_BBT_MARK_NADDR_MAX))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX/2)", test_BBT_MARK_NADDR_MAX2))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX/4)", test_BBT_MARK_NADDR_MAX4))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=1)", test_BBT_MARK_NADDR_1))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_set", test_BBT_SET))
		goto out;

	if (!CU_add_test(pSuite, "nvm_bbt_get CACHED", test_BBT_GET_CACHED))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX, CACHED)", test_BBT_MARK_NADDR_MAX_CACHED))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX/2, CACHED)", test_BBT_MARK_NADDR_MAX2_CACHED))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=MAX/4, CACHED)", test_BBT_MARK_NADDR_MAX4_CACHED))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_mark (NADDR=1, CACHED)", test_BBT_MARK_NADDR_1_CACHED))
		goto out;
	if (!CU_add_test(pSuite, "nvm_bbt_set CACHED", test_BBT_SET_CACHED))
		goto out;

	switch(rmode) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(rmode);
		CU_basic_run_tests();
		break;
	}

out:
	err = CU_get_error() || \
	      CU_get_number_of_suites_failed() || \
	      CU_get_number_of_tests_failed() || \
	      CU_get_number_of_failures();

	CU_cleanup_registry();

	return err;
}
