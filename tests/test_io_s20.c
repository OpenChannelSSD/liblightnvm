#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

size_t compare_buffers(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	for (size_t i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			++diff;
		}
	}

	return diff;
}

void print_mismatch(char *expected, char *actual, size_t nbytes)
{
	printf("MISMATCHES:\n");
	for (size_t i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			printf("i(%06lu), expected(%c) != actual(%02d|0x%02x|%c)\n",
				i, expected[i], (int)actual[i], (int)actual[i], actual[i]);
		}
	}
}

int setup(void)
{
	return 0;
}

int teardown(void)
{
	return 0;
}

#define WS_MIN 12

void TEST_NADDR_META_OFF(void)
{
	struct nvm_dev *dev = NULL;
	const struct nvm_geo *geo = NULL;
	const struct nvm_spec_lgeo *lgeo = NULL;

	size_t buf_nbytes = 0;
	char *buf_w = NULL;
	char *buf_r = NULL;

	const char dev_path[] = "traddr:0000:02:00.0";
	const int pugrp = 4;
	const int punit = 0;
	const int chunk = 0;

	struct nvm_addr addrs[NVM_NADDR_MAX];

	// Setup device
	{
		dev = nvm_dev_open(dev_path);
		if (!dev) {
			CU_FAIL("nvm_buf_alloc");
			goto out;
		}

		geo = nvm_dev_get_geo(dev);
		if (!geo) {
			CU_FAIL("nvm_dev_get_geo");
			goto out;
		}

		lgeo = nvm_dev_get_lgeo(dev);
		if (!lgeo) {
			CU_FAIL("nvm_dev_get_lgeo");
			goto out;
		}
	}

	// Setup buffers
	{
		buf_nbytes = lgeo->nbytes * lgeo->nsectr;

		buf_w = nvm_buf_alloc(geo, buf_nbytes);
		if (!buf_w) {
			goto out;
		}
		nvm_buf_fill(buf_w, buf_nbytes);
		
		buf_r = nvm_buf_alloc(geo, buf_nbytes);
		if (!buf_r) {
			goto out;
		}
		memset(buf_r, 0, buf_nbytes);
	}

	// ERASE
	{
		addrs[0].l.pugrp = pugrp;
		addrs[0].l.punit = punit,
		addrs[0].l.chunk = chunk,
		addrs[0].l.sectr = 0;

		if (nvm_cmd_erase(dev, addrs, 1, 0, NULL)) {
			CU_FAIL("nvm_cmd_erase(...)");
			goto out;
		}
	}

	// WRITE
	for (unsigned int sectr = 0; sectr < lgeo->nsectr; sectr += WS_MIN)
	{
		for (unsigned int i = 0; i < WS_MIN; ++i) {
			addrs[i].l.pugrp = pugrp;
			addrs[i].l.punit = punit,
			addrs[i].l.chunk = chunk,
			addrs[i].l.sectr = sectr + i;
		}

		if (nvm_cmd_write(dev, addrs, WS_MIN, buf_w, NULL, 0x0, NULL)) {
			CU_FAIL("nvm_cmd_write");
			goto out;
		}
	}

	// READ
	for (unsigned int sectr = 0; sectr < lgeo->nsectr; sectr += WS_MIN)
	{
		for (unsigned int i = 0; i < WS_MIN; ++i) {
			addrs[i].l.pugrp = pugrp;
			addrs[i].l.punit = punit,
			addrs[i].l.chunk = chunk,
			addrs[i].l.sectr = sectr + i;
		}

		if (nvm_cmd_read(dev, addrs, WS_MIN, buf_r, NULL, 0x0, NULL)) {
			CU_FAIL("nvm_cmd_read");
			goto out;
		}
	}

	CU_PASS("Success");

out:
	free(buf_w);
	free(buf_r);
	nvm_dev_close(dev);
	return;
}

int main(int argc, char **argv)
{
	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_addr_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "NADDR_META_OFF", TEST_NADDR_META_OFF)) ||
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
