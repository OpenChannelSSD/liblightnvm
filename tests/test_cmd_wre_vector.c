#include "test_intf.c"

#define NBYTES_QRK 4

size_t nvm_buf_diff_qrk(char *expected, char *actual, size_t nbytes,
			size_t nbytes_oob,
			size_t nbytes_qrk)
{
	size_t diff = 0;

	for (size_t i = 0; i < nbytes; ++i) {
		if (nbytes_oob && ((i % nbytes_oob) < nbytes_qrk))
			continue;

		if (expected[i] != actual[i])
			++diff;
	}

	return diff;
}

void ewr_s12_1addr(int use_meta)
{
	char *buf_w = NULL, *buf_r = NULL, *meta_w = NULL, *meta_r = NULL;
	const int naddrs = GEO->nplanes * GEO->nsectors;
	struct nvm_addr addrs[naddrs];
	struct nvm_addr blk_addr = { .val = 0 };
	struct nvm_ret ret;
	ssize_t res;
	size_t buf_w_nbytes, meta_w_nbytes, buf_r_nbytes, meta_r_nbytes;
	int pmode = NVM_FLAG_PMODE_SNGL;
	int failed = 1;

	if (nvm_cmd_gbbt_arbs(DEV, NVM_BBT_FREE, 1, &blk_addr)) {
		CU_FAIL("nvm_cmd_gbbt_arbs");
		goto out;
	}

	buf_w_nbytes = naddrs * GEO->sector_nbytes;
	meta_w_nbytes = naddrs * GEO->meta_nbytes;
	buf_r_nbytes = GEO->sector_nbytes;
	meta_r_nbytes = GEO->meta_nbytes;

	buf_w = nvm_buf_alloc(DEV, buf_w_nbytes, NULL);	// Setup buffers
	if (!buf_w) {
		CU_FAIL("nvm_buf_alloc");
		goto out;
	}
	nvm_buf_fill(buf_w, buf_w_nbytes);

	meta_w = nvm_buf_alloc(DEV, meta_w_nbytes, NULL);
	if (!meta_w) {
		CU_FAIL("nvm_buf_alloc");
		goto out;
	}
	for (size_t i = 0; i < meta_w_nbytes; ++i) {
		meta_w[i] = 65;
	}
	for (int i = 0; i < naddrs; ++i) {
		char meta_descr[meta_w_nbytes];
		int sec = i % GEO->nsectors;
		int pl = (i / GEO->nsectors) % GEO->nplanes;

		sprintf(meta_descr, "[P(%02d),S(%02d)]", pl, sec);
		if (strlen(meta_descr) > GEO->meta_nbytes) {
			CU_FAIL("Failed constructing meta buffer");
			goto out;
		}

		memcpy(meta_w + i * GEO->meta_nbytes, meta_descr,
		       strlen(meta_descr));
	}

	buf_r = nvm_buf_alloc(DEV, buf_r_nbytes, NULL);
	if (!buf_r) {
		CU_FAIL("nvm_buf_alloc");
		goto out;
	}

	meta_r = nvm_buf_alloc(DEV, meta_r_nbytes, NULL);
	if (!meta_r) {
		CU_FAIL("nvm_buf_alloc");
		goto out;
	}

	if (pmode) {
		addrs[0].ppa = blk_addr.ppa;
	} else {
		for (size_t pl = 0; pl < GEO->nplanes; ++pl) {
			addrs[pl].ppa = blk_addr.ppa;

			addrs[pl].g.pl = pl;
		}
	}

	res = nvm_cmd_erase(DEV, addrs, pmode ? 1 : GEO->nplanes, NULL, pmode,
			    &ret);
	if (res < 0) {
		CU_FAIL("Erase failure");
		goto out;
	}

	for (size_t pg = 0; pg < GEO->npages; ++pg) {
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.sec = i % GEO->nsectors;
			addrs[i].g.pl = (i / GEO->nsectors) % GEO->nplanes;
		}
		res = nvm_cmd_write(DEV, addrs, naddrs, buf_w,
				     use_meta ? meta_w : NULL, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto out;
		}
	}

	for (size_t pg = 0; pg < GEO->npages; ++pg) {
		for (size_t pl = 0; pl < GEO->nplanes; ++pl) {
			for (size_t sec = 0; sec < GEO->nsectors; ++sec) {
				struct nvm_addr addr;
				size_t buf_diff = 0, meta_diff = 0;

				int bw_offset = sec * GEO->sector_nbytes + \
						pl * GEO->nsectors * \
						GEO->sector_nbytes;
				int mw_offset = sec * GEO->meta_nbytes + \
						pl * GEO->nsectors * \
						GEO->meta_nbytes;

				addr.ppa = blk_addr.ppa;
				addr.g.pg = pg;
				addr.g.pl = pl;
				addr.g.sec = sec;

				memset(buf_r, 0, buf_r_nbytes);
				if (use_meta)
					memset(meta_r, 0, meta_r_nbytes);

				res = nvm_cmd_read(DEV, &addr, 1, buf_r,
						    use_meta ? meta_r : NULL, pmode, &ret);
				if (res < 0) {
					CU_FAIL("Read failure");
					goto out;
				}

				buf_diff = nvm_buf_diff_qrk(buf_r,
							    buf_w + bw_offset,
							    buf_r_nbytes,
							    GEO->g.meta_nbytes,
							    NBYTES_QRK);
				if (use_meta)
					meta_diff = nvm_buf_diff_qrk(meta_r,
							meta_w + mw_offset,
							meta_r_nbytes,
							GEO->g.meta_nbytes,
							NBYTES_QRK);

				if (buf_diff)
					CU_FAIL("Read failure: buffer mismatch");
				if (use_meta && meta_diff) {
					CU_FAIL("Read failure: meta mismatch");
					if (CU_BRM_VERBOSE == RMODE) {
						nvm_buf_diff_pr(meta_w + mw_offset,
								meta_r,
								meta_r_nbytes);
					}
				}
				if (buf_diff || meta_diff)
					goto out;
			}
		}
	}

	failed = 0;
	CU_PASS("Success");

out:
	if ((CU_BRM_VERBOSE == RMODE) && failed) {
		printf("\n# Failed using\n");
		nvm_addr_prn(&blk_addr, 1, DEV);
	}

	nvm_buf_free(DEV, meta_r);
	nvm_buf_free(DEV, buf_r);
	nvm_buf_free(DEV, meta_w);
	nvm_buf_free(DEV, buf_w);
}

void test_EWR_S12_1ADDR_META0_SNGL(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		ewr_s12_1addr(0);
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_S12_1ADDR_META1_SNGL(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		ewr_s12_1addr(1);
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void ewr_s12_naddr(int use_meta, int pmode)
{
	const int naddrs = GEO->nplanes * GEO->nsectors;
	struct nvm_addr blk_addr = { .val = 0 };
	struct nvm_addr addrs[naddrs];
	struct nvm_buf_set *bufs = NULL;
	struct nvm_ret ret;
	ssize_t res;
	int failed = 1;

	if (nvm_cmd_gbbt_arbs(DEV, NVM_BBT_FREE, 1, &blk_addr)) {
		CU_FAIL("nvm_cmd_gbbt_arbs");
		goto out;
	}

	if (CU_BRM_VERBOSE == RMODE) {
		printf("# using addr\n");
		nvm_addr_prn(&blk_addr, 1, DEV);
	}

	bufs = nvm_buf_set_alloc(DEV, naddrs * GEO->g.sector_nbytes,
				 use_meta ? naddrs * GEO->g.meta_nbytes : 0);
	if (!bufs) {
		CU_FAIL("nvm_buf_set_alloc");
		goto out;
	}
	nvm_buf_set_fill(bufs);

	// TODO: This should do quirks-testing
	if (pmode) {					///< Erase
		addrs[0].ppa = blk_addr.ppa;
		res = nvm_cmd_erase(DEV, addrs, 1, NULL, pmode, &ret);
	} else {
		for (size_t pl = 0; pl < GEO->nplanes; ++pl) {
			addrs[pl].ppa = blk_addr.ppa;

			addrs[pl].g.pl = pl;
		}
		res = nvm_cmd_erase(DEV, addrs, GEO->nplanes, NULL, pmode, &ret);
	}
	if (res < 0) {
		CU_FAIL("Erase failure");
		goto out;
	}

	for (size_t pg = 0; pg < GEO->npages; ++pg) {	///< Write
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].ppa = blk_addr.ppa;

			addrs[i].g.pg = pg;
			addrs[i].g.sec = i % GEO->nsectors;
			addrs[i].g.pl = (i / GEO->nsectors) % GEO->nplanes;
		}
		res = nvm_cmd_write(DEV, addrs, naddrs, bufs->write,
				    bufs->write_meta, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto out;
		}
	}

	for (size_t pg = 0; pg < GEO->npages; ++pg) {	///< Read
		size_t buf_diff = 0, meta_diff = 0;

		for (int i = 0; i < naddrs; ++i) {
			addrs[i].val = blk_addr.val;

			addrs[i].g.pg = pg;
			addrs[i].g.pl = (i / GEO->g.nsectors) % GEO->g.nplanes;
			addrs[i].g.sec = i % GEO->g.nsectors;
		}

		memset(bufs->read, 0, bufs->nbytes);	///< Reset read buffers
		if (use_meta)
			memset(bufs->read_meta, 0, bufs->nbytes_meta);

		res = nvm_cmd_read(DEV, addrs, naddrs, bufs->read,
				   bufs->read_meta, pmode, &ret);
		if (res < 0) {
			CU_FAIL("Read failure: command error");
			goto out;
		}

		buf_diff = nvm_buf_diff_qrk(bufs->write, bufs->read,
					    bufs->nbytes,
					    GEO->g.meta_nbytes,
					    NBYTES_QRK);
		if (use_meta)
			meta_diff = nvm_buf_diff_qrk(bufs->write_meta,
						bufs->read_meta,
						bufs->nbytes_meta,
						GEO->g.meta_nbytes,
						NBYTES_QRK);

		if (buf_diff)
			CU_FAIL("Read failure: buffer mismatch");
		if (use_meta && meta_diff) {
			CU_FAIL("Read failure: meta mismatch");
			if (CU_BRM_VERBOSE == RMODE) {
				nvm_buf_diff_pr(bufs->write_meta,
						bufs->read_meta,
						bufs->nbytes_meta);
			}
		}
		if (buf_diff || meta_diff)
			goto out;
	}

	failed = 0;
	CU_PASS("Success");

out:
	if ((CU_BRM_VERBOSE == RMODE) && failed) {
		printf("\n# Failed using\n");
		nvm_addr_prn(&blk_addr, 1, DEV);
	}

	nvm_buf_set_free(bufs);

	return;
}

void test_EWR_S12_NADDR_META0_SNGL(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		ewr_s12_naddr(0, NVM_FLAG_PMODE_SNGL);
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_S12_NADDR_META1_SNGL(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		ewr_s12_naddr(1, NVM_FLAG_PMODE_SNGL);
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_S12_NADDR_META0_DUAL(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		if (GEO->nplanes >= 2) {
			ewr_s12_naddr(0, NVM_FLAG_PMODE_DUAL);
		} else {
			CU_PASS("Nothing to test");
		}
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_S12_NADDR_META1_DUAL(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		if (GEO->nplanes >= 2) {
			ewr_s12_naddr(1, NVM_FLAG_PMODE_DUAL);
		} else {
			CU_PASS("Nothing to test");
		}
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_S12_NADDR_META0_QUAD(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		if (GEO->nplanes >= 4) {
			ewr_s12_naddr(0, NVM_FLAG_PMODE_QUAD);
		} else {
			CU_PASS("Nothing to test");
		}
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

void test_EWR_S12_NADDR_META1_QUAD(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		if (GEO->nplanes >= 4) {
			ewr_s12_naddr(1, NVM_FLAG_PMODE_QUAD);
		} else {
			CU_PASS("Nothing to test");
		}
		break;

	case NVM_SPEC_VERID_20:
		CU_PASS("Nothing to test");
		break;

	default:
		CU_FAIL("Invalid VERID");
	}
}

// Erase, write, and read an single chunk
static void ewr_s20(int use_rwmeta, int use_erase_meta)
{
	const int naddrs = nvm_dev_get_ws_min(DEV);
	struct nvm_addr addrs[naddrs];
	struct nvm_buf_set *bufs = NULL;
	struct nvm_spec_rprt_descr *erase_meta = NULL;
	struct nvm_ret ret;
	struct nvm_addr chunk_addr = { .val = 0 };
	ssize_t res;

	if (nvm_cmd_rprt_arbs(DEV, NVM_CHUNK_STATE_FREE, 1, &chunk_addr)) {
		CU_FAIL("nvm_cmd_rprt_arbs");
		goto out;
	}

	bufs = nvm_buf_set_alloc(DEV, naddrs * GEO->l.nbytes,
				 use_rwmeta ? naddrs * GEO->l.nbytes_oob : 0);
	if (!bufs) {
		CU_FAIL("nvm_buf_set_alloc");
		goto out;
	}
	nvm_buf_set_fill(bufs);

	if (use_erase_meta) {
		erase_meta = nvm_buf_alloc(DEV, naddrs * sizeof(*erase_meta), NULL);
		if (!erase_meta) {
			CU_FAIL("nvm_buf_alloc for erase_meta");
			goto out;
		}
	}
								///< Write
	for (size_t sectr = 0; sectr < GEO->l.nsectr; sectr += naddrs) {
		for (int i = 0; i < naddrs; ++i) {
			addrs[i].val = chunk_addr.val;
			addrs[i].l.sectr = sectr + i;
		}
		res = nvm_cmd_write(DEV, addrs, naddrs, bufs->write,
				    bufs->write_meta, 0x0, &ret);
		if (res < 0) {
			CU_FAIL("Write failure");
			goto out;
		}
	}

								///< Read
	for (size_t sectr = 0; sectr < GEO->l.nsectr; sectr += naddrs) {
		size_t buf_diff = 0;
		size_t meta_diff = 0;

		for (int i = 0; i < naddrs; ++i) {
			addrs[i].val = chunk_addr.val;
			addrs[i].l.sectr = sectr + i;
		}

		memset(bufs->read, 0, bufs->nbytes);	///< Reset read buffers
		if (use_rwmeta)
			memset(bufs->read_meta, 0, bufs->nbytes_meta);

		res = nvm_cmd_read(DEV, addrs, naddrs, bufs->read,
				   bufs->read_meta, 0x0, &ret);
		if (res < 0) {
			CU_FAIL("Read failure: command error");
			goto out;
		}

		buf_diff = nvm_buf_diff_qrk(bufs->read, bufs->write, bufs->nbytes,
					    GEO->l.nbytes_oob,
					    NBYTES_QRK);
		if (buf_diff) {
			CU_FAIL("Read failure: buffer mismatch");
		}

		if (use_rwmeta) {
			meta_diff = nvm_buf_diff_qrk(bufs->read_meta,
						 bufs->write_meta,
						 bufs->nbytes_meta,
					    GEO->l.nbytes_oob,
					    NBYTES_QRK);
			if (meta_diff) {
				CU_FAIL("Read failure: meta mismatch");
				if (CU_BRM_VERBOSE == RMODE) {
					nvm_buf_diff_pr(bufs->write_meta,
							bufs->read_meta,
							bufs->nbytes_meta);
				}
			}
		}

		if (buf_diff || meta_diff) {
			CU_FAIL("buf_diff || meta_diff");
			goto out;
		}
	}

	res = nvm_cmd_erase(DEV, &chunk_addr, 1, erase_meta, 0x0, &ret); ///< Erase
	if (res < 0) {
		CU_FAIL("Erase failure");
		goto out;
	}

	if (use_erase_meta) {
		CU_ASSERT(erase_meta->cs == NVM_CHUNK_STATE_FREE);
		CU_ASSERT(erase_meta->wp == 0);
		CU_ASSERT(erase_meta->naddrs == GEO->l.nsectr);
		CU_ASSERT(erase_meta->addr == nvm_addr_gen2dev(DEV, chunk_addr));
	}

	CU_PASS("Success");

out:
	nvm_buf_set_free(bufs);
	nvm_buf_free(DEV, erase_meta);
}

void test_EWR_S20_RWMETA0_EMETA0(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr_s20(0, 0);
		break;

	default:
		CU_FAIL("invalid verid");
	}
}

void test_EWR_S20_RWMETA1_EMETA0(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr_s20(1, 0);
		break;

	default:
		CU_FAIL("invalid verid");
	}
}

void test_EWR_S20_RWMETA0_EMETA1(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr_s20(0, 1);
		break;

	default:
		CU_FAIL("invalid verid");
	}
}

void test_EWR_S20_RWMETA1_EMETA1(void)
{
	switch(nvm_dev_get_verid(DEV)) {
	case NVM_SPEC_VERID_12:
		CU_PASS("nothing to test");
		break;

	case NVM_SPEC_VERID_20:
		ewr_s20(1, 1);
		break;

	default:
		CU_FAIL("invalid verid");
	}
}

int main(int argc, char **argv)
{
	int err = 0;

	CU_pSuite pSuite = suite_create("nvm_test_cmd_wre_vector",
					argc, argv);
	if (!pSuite)
		goto out;

	if (!CU_add_test(pSuite, "EWR_S20_RWMETA0_EMETA0", test_EWR_S20_RWMETA0_EMETA0))
		goto out;
	if (!CU_add_test(pSuite, "EWR_S20_RWMETA1_EMETA0", test_EWR_S20_RWMETA1_EMETA0))
		goto out;
	if (!CU_add_test(pSuite, "EWR_S20_RWMETA0_EMETA1", test_EWR_S20_RWMETA0_EMETA1))
		goto out;
	if (!CU_add_test(pSuite, "EWR_S20_RWMETA1_EMETA1", test_EWR_S20_RWMETA1_EMETA1))
		goto out;

	if (!CU_add_test(pSuite, "EWR S12 - META NADDR QUAD", test_EWR_S12_NADDR_META0_QUAD))
		goto out;
	if (!CU_add_test(pSuite, "EWR S12 - META NADDR DUAL", test_EWR_S12_NADDR_META0_DUAL))
		goto out;
	if (!CU_add_test(pSuite, "EWR S12 - META NADDR SNGL", test_EWR_S12_NADDR_META0_SNGL))
		goto out;
	if (!CU_add_test(pSuite, "EWR S12 - META 1ADDR SNGL", test_EWR_S12_1ADDR_META0_SNGL))
		goto out;

	if (!CU_add_test(pSuite, "EWR S12 + META NADDR QUAD", test_EWR_S12_NADDR_META1_QUAD))
		goto out;
	if (!CU_add_test(pSuite, "EWR S12 + META NADDR DUAL", test_EWR_S12_NADDR_META1_DUAL))
		goto out;
	if (!CU_add_test(pSuite, "EWR S12 + META NADDR SNGL", test_EWR_S12_NADDR_META1_SNGL))
		goto out;
	if (!CU_add_test(pSuite, "EWR S12 + META 1ADDR SNGL", test_EWR_S12_1ADDR_META1_SNGL))
		goto out;

	switch(RMODE) {
	case NVM_TEST_RMODE_AUTO:
		CU_automated_run_tests();
		break;

	default:
		CU_basic_set_mode(RMODE);
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
