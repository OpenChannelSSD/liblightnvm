/*
 * append_tests - Tests for append storage interface
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

static char nvm_dev[DISK_NAME_LEN];
static char nvm_tgt_type[NVM_TTYPE_NAME_MAX] = "dflash";
static char nvm_tgt_name[DISK_NAME_LEN] = "llnbeamtest0";

static void init_data_test(char *buf, size_t len)
{
	size_t i;
	char payload = 'a';

	for (i = 0; i < len; i++) {
		buf[i] = (payload + i) % 28;
	}
}

#define TEST_AR_ALL 1
#define TEST_AR_1K_BYTES 2
#define TEST_AR_1_BYTE 4
void beam_ar_generic(char *src, char *dst, size_t len, int flags)
{
	size_t written, read;
	int beam_id;
	int ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev, 0, 0);
	CU_ASSERT(ret == 0);

	beam_id = nvm_beam_create(nvm_tgt_name, 0, 0);
	CU_ASSERT(beam_id > 0);

	written = nvm_beam_append(beam_id, src, len);
	CU_ASSERT(len == written);

	ret = nvm_beam_sync(beam_id, 0);
	CU_ASSERT(0 == ret);

	read = nvm_beam_read(beam_id, dst, written, 0, 0);
	CU_ASSERT(written == read);

	CU_ASSERT_STRING_EQUAL(src, dst);

	if (flags & TEST_AR_1K_BYTES) {
		/* read in 1000 byte chunks */
		memset(dst, 0, len);

		for (int i = 0; i < len / 1000; i++) {
			int offset = i * 1000;
			read = nvm_beam_read(beam_id, dst + offset, 1000,
								offset, 0);
			CU_ASSERT(1000 == read);
		}
		CU_ASSERT_NSTRING_EQUAL(src, dst, len);
	}

	if (flags & TEST_AR_1_BYTE) {
		/* read in 1 byte chunks */
		memset(dst, 0, len);

		for (int i = 0; i < len; i++) {
			read = nvm_beam_read(beam_id, dst + i, 1, i, 0);
			CU_ASSERT(1 == read);
		}
		CU_ASSERT_NSTRING_EQUAL(src, dst, len);
	}

	nvm_beam_destroy(beam_id, 0);

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0 == ret);
}

int init_suite1(void)
{
	return nvm_beam_init();
}

int clean_suite1(void)
{
	nvm_beam_exit();
	return 0;
}

void test_CREATE_BEAM(void)
{
	int beam_id;
	int ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev, 0, 0);
	CU_ASSERT(0 == ret);

	beam_id = nvm_beam_create(nvm_tgt_name, 0, 0);
	CU_ASSERT(beam_id > 0);

	nvm_beam_destroy(beam_id, 0);

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0 == ret);
}

void test_BEAM_CLOSE_UNGRACEFUL(void)
{
	int beam_id, ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev, 0, 0);
	CU_ASSERT(ret == 0);

	beam_id = nvm_beam_create(nvm_tgt_name, 0, 0);
	CU_ASSERT(beam_id > 0);

	nvm_beam_destroy(beam_id, 0);

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0 == ret);

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev, 0, 0);
	CU_ASSERT(ret == 0);

	beam_id = nvm_beam_create(nvm_tgt_name, 0, 0);
	CU_ASSERT(beam_id > 0);

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0 == ret);
}

/*
 * Append and read back from beam.
 *	- Append: payload < PAGE_SIZE
 *	- Read: All
 *	- open - append - close - open - read - close
 */
void test_BEAM_AR1(void)
{
	char test[100] = "Hello World\n";
	int test_len = strlen(test);
	char test2[100];

	size_t written, read;
	int beam_id;
	int ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev, 0, 0);
	CU_ASSERT(ret == 0);

	beam_id = nvm_beam_create(nvm_tgt_name, 0, 0);
	CU_ASSERT(beam_id > 0);

	written = nvm_beam_append(beam_id, test, test_len+1);
	CU_ASSERT(test_len+1 == written);

	ret = nvm_beam_sync(beam_id, 0);
	CU_ASSERT(0 == ret);

	read = nvm_beam_read(beam_id, test2, written, 0, 0);
	CU_ASSERT(written == read);

	CU_ASSERT_STRING_EQUAL(test, test2);

	nvm_beam_destroy(beam_id, 0);

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0 == ret);
}

/*
 * Append and read back from beam.
 *	- Append: PAGE_SIZE < payload < BLOCK_SIZE
 *	- Read: All, 1000 byte chunks, 1 byte chunks
 *	- open - append - close - open - read - close
 */
void test_BEAM_AR2(void)
{
	char test[5000];
	char test2[5000];
	int flags = TEST_AR_ALL | TEST_AR_1K_BYTES | TEST_AR_1_BYTE;

	init_data_test(test, 5000);
	beam_ar_generic(test, test2, 5000, flags);
}

/*
 * Append and read back from beam.
 *	- Append: payload is multiple pages in same block
 *	- Read: All, 1000 byte chunks, 1 byte chunks
 *	- open - append - close - open - read - close
 */
void test_BEAM_AR3(void)
{
	char test[50000];
	char test2[50000];
	int flags = TEST_AR_ALL | TEST_AR_1K_BYTES | TEST_AR_1_BYTE;

	init_data_test(test, 50000);
	beam_ar_generic(test, test2, 50000, flags);
}

/*
 * Append and read back from beam.
 *	- Append: payload > BLOCK_SIZE
 *	- Read: All, 1000 byte chunks, 1 byte chunks
 *	- open - append - close - open - read - close
 */
void test_BEAM_AR4(void)
{
	size_t test_size = 2000000;
	char test[test_size];
	char test2[test_size];
	int flags = TEST_AR_ALL | TEST_AR_1K_BYTES;

	init_data_test(test, test_size);
	beam_ar_generic(test, test2, test_size, flags);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s nvm_dev / nvm_dev: LightNVM device\n",
									argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("Argument nvm_dev can be maximum %d characters\n",
							DISK_NAME_LEN - 1);
	}

	strcpy(nvm_dev, argv[1]);

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_beam*", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "beam_create()", test_CREATE_BEAM)) ||
	(NULL == CU_add_test(pSuite, "beam ungraceful close", test_BEAM_CLOSE_UNGRACEFUL)) ||
	(NULL == CU_add_test(pSuite, "beam AR1", test_BEAM_AR1)) ||
	(NULL == CU_add_test(pSuite, "beam AR2", test_BEAM_AR2)) ||
	(NULL == CU_add_test(pSuite, "beam AR3", test_BEAM_AR3)) ||
	(NULL == CU_add_test(pSuite, "beam AR4", test_BEAM_AR4)) ||
	0)
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
