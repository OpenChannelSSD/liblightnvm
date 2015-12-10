/*
 * append_tests - Tests for append storage interface
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <liblightnvm.h>
#include <unistd.h>

#include "CuTest/CuTest.h"
#include "../src/ioctl.h"

static CuSuite *per_test_suite = NULL;
static char lnvm_dev[DISK_NAME_LEN];
static char lnvm_tgt_type[NVM_TTYPE_NAME_MAX] = "dflash";
static char lnvm_tgt_name[DISK_NAME_LEN] = "liblnvm_test";

static void init_lib(CuTest *ct)
{
	int ret = nvm_init();
	CuAssertIntEquals(ct, 0, ret);
}

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

static void create_beam(CuTest *ct)
{
	int tgt_id, beam_id;
	int i;

	create_tgt(ct);

	beam_id = nvm_beam_create(lnvm_tgt_name, 0, 0);
	CuAssertTrue(ct, beam_id > 0);

	nvm_beam_destroy(beam_id, 0);

	remove_tgt(ct);
}

static void beam_close_ungrac(CuTest *ct)
{
	int tgt_id, beam_id;

	create_tgt(ct);

	beam_id = nvm_beam_create(lnvm_tgt_name, 0, 0);
	CuAssertTrue(ct, beam_id > 0);

	nvm_beam_destroy(beam_id, 0);

	remove_tgt(ct);

	create_tgt(ct);

	beam_id = nvm_beam_create(lnvm_tgt_name, 0, 0);
	CuAssertTrue(ct, beam_id > 0);

	remove_tgt(ct);
}

/*
 * Append and read back from beam.
 *	- Append: payload < PAGE_SIZE
 *	- Read: All
 *	- open - append - close - open - read - close
 */
static void beam_ar1(CuTest *ct)
{
	size_t written, read;
	char test[100] = "Hello World\n";
	char test2[100];
	int tgt_id, beam_id;
	int ret;

	create_tgt(ct);

	beam_id = nvm_beam_create(lnvm_tgt_name, 0, 0);
	CuAssertTrue(ct, beam_id > 0);

	written = nvm_beam_append(beam_id, test, 12);
	CuAssertIntEquals(ct, 12, written);

	ret = nvm_beam_sync(beam_id, 0);
	CuAssertIntEquals(ct, 0, ret);

	read = nvm_beam_read(beam_id, test2, written, 0, 0);
	CuAssertIntEquals(ct, written, read);

	CuAssertByteArrayEquals(ct, test, test2, 12, 12);

	nvm_beam_destroy(beam_id, 0);

	remove_tgt(ct);
}

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
static void beam_ar_generic(CuTest *ct, char *src, char *dst, size_t len,
								int flags)
{
	size_t written, read;
	int tgt_id, beam_id;
	int ret;
	size_t i;

	create_tgt(ct);

	beam_id = nvm_beam_create(lnvm_tgt_name, 0, 0);
	CuAssertTrue(ct, beam_id > 0);

	written = nvm_beam_append(beam_id, src, len);
	CuAssertIntEquals(ct, len, written);

	ret = nvm_beam_sync(beam_id, 0);
	CuAssertIntEquals(ct, 0, ret);

	read = nvm_beam_read(beam_id, dst, written, 0, 0);
	CuAssertIntEquals(ct, written, read);

	CuAssertByteArrayEquals(ct, src, dst, len, len);

	if (flags & TEST_AR_1K_BYTES) {
		/* read in 1000 byte chunks */
		memset(dst, 0, len);

		for (i = 0; i < len / 1000; i++) {
			int offset = i * 1000;
			read = nvm_beam_read(beam_id, dst + offset, 1000,
								offset, 0);
			CuAssertIntEquals(ct, 1000, read);
		}

		CuAssertByteArrayEquals(ct, src, dst, len, len);
	}

	if (flags & TEST_AR_1_BYTE) {
		/* read in 1 byte chunks */
		memset(dst, 0, len);

		for (i = 0; i < len; i++) {
			read = nvm_beam_read(beam_id, dst + i, 1, i, 0);
			CuAssertIntEquals(ct, 1, read);
		}

		CuAssertByteArrayEquals(ct, src, dst, len, len);
	}

	nvm_beam_destroy(beam_id, 0);

	remove_tgt(ct);
}

/*
 * Append and read back from beam.
 *	- Append: PAGE_SIZE < payload < BLOCK_SIZE
 *	- Read: All, 1000 byte chunks, 1 byte chunks
 *	- open - append - close - open - read - close
 */
static void beam_ar2(CuTest *ct)
{
	char test[5000];
	char test2[5000];
	int flags = TEST_AR_ALL | TEST_AR_1K_BYTES | TEST_AR_1_BYTE;

	init_data_test(test, 5000);
	beam_ar_generic(ct, test, test2, 5000, flags);
}

/*
 * Append and read back from beam.
 *	- Append: payload is multiple pages in same block
 *	- Read: All, 1000 byte chunks, 1 byte chunks
 *	- open - append - close - open - read - close
 */
static void beam_ar3(CuTest *ct)
{
	char test[50000];
	char test2[50000];
	int flags = TEST_AR_ALL | TEST_AR_1K_BYTES | TEST_AR_1_BYTE;

	init_data_test(test, 50000);
	beam_ar_generic(ct, test, test2, 50000, flags);
}

/*
 * Append and read back from beam.
 *	- Append: payload > BLOCK_SIZE
 *	- Read: All, 1000 byte chunks, 1 byte chunks
 *	- open - append - close - open - read - close
 */
static void beam_ar4(CuTest *ct)
{
	size_t test_size = 2000000;
	char test[test_size];
	char test2[test_size];
	int flags = TEST_AR_ALL | TEST_AR_1K_BYTES;

	init_data_test(test, test_size);
	beam_ar_generic(ct, test, test2, test_size, flags);
}

static void exit_lib(CuTest *ct)
{
	nvm_exit();
}

CuSuite *append_GetSuite()
{
	per_test_suite = CuSuiteNew();

	SUITE_ADD_TEST(per_test_suite, init_lib);
	SUITE_ADD_TEST(per_test_suite, create_tgt);
	SUITE_ADD_TEST(per_test_suite, remove_tgt);
	SUITE_ADD_TEST(per_test_suite, create_beam);
	SUITE_ADD_TEST(per_test_suite, beam_close_ungrac);
	SUITE_ADD_TEST(per_test_suite, beam_ar1);
	SUITE_ADD_TEST(per_test_suite, beam_ar2);
	SUITE_ADD_TEST(per_test_suite, beam_ar3);
	SUITE_ADD_TEST(per_test_suite, beam_ar4);
	SUITE_ADD_TEST(per_test_suite, exit_lib);
}

void run_all_test(void)
{
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();

	CuSuiteAddSuite(suite, (CuSuite*) append_GetSuite());

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
