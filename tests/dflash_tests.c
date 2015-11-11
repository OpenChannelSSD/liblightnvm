/*
 * dflash - user-space append-only file system for flash memories
 *
 * Copyright (C) 2015 Javier Gonzalez <javier@cnexlabs.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#include "stdio.h"
#include "stdlib.h"

#include <liblightnvm.h>
#include "CuTest/CuTest.h"
#include "../src/ioctl.h"

static CuSuite *per_test_suite = NULL;
uint64_t file_id;

static void init_lib(CuTest *ct)
{
	int ret = nvm_init();
	CuAssertIntEquals(ct, 0, ret);
}

static void create_tgt(CuTest *ct)
{
	struct nvm_ioctl_create c;
	int ret;

	sprintf(c.dev, "nvme0n1");
	sprintf(c.tgttype, "dflash");
	sprintf(c.tgtname, "test1");
	c.flags = 0;
	c.conf.type = 0;
	c.conf.s.lun_begin = 0;
	c.conf.s.lun_end = 0;

	ret = nvm_create_target(&c);

	CuAssertIntEquals(ct, 0, ret);
}

static void remove_tgt(CuTest *ct)
{
	struct nvm_ioctl_remove c;
	int ret;

	sprintf(c.tgtname, "test1");
	c.flags = 0;

	ret = nvm_remove_target(&c);

	CuAssertIntEquals(ct, 0, ret);
}

static void create_file(CuTest *ct)
{
	int tgt_id, fd;
	int i;

	for (i = 0; i < 20; i++) {
	create_tgt(ct);
	tgt_id = nvm_open_target("test1", 0);
	CuAssertTrue(ct, tgt_id > 0);

	file_id = nvm_file_create(tgt_id, 0, 0);
	CuAssertTrue(ct, file_id > 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	nvm_file_close(fd, 0);
	nvm_file_delete(file_id, 0);
	nvm_close_target(tgt_id);

	/* sleep(1); */
	remove_tgt(ct);
	}
}

static void append_file(CuTest *ct)
{
	size_t written;
	char test[100] = "Hello World";
	int tgt_id, fd;
	int i;

	/* for (i = 0; i < 20; i++) { */
	create_tgt(ct);
	tgt_id = nvm_open_target("test1", 0);
	CuAssertTrue(ct, tgt_id > 0);

	file_id = nvm_file_create(tgt_id, 0, 0);
	CuAssertTrue(ct, file_id > 0);

	fd = nvm_file_open(file_id, 0);
	CuAssertTrue(ct, fd > 0);

	written = nvm_file_append(fd, test, 11);
	CuAssertIntEquals(ct, 11, written);

	nvm_file_close(fd, 0);
	nvm_file_delete(file_id, 0);
	nvm_close_target(tgt_id);

	sleep(1);
	remove_tgt(ct);
	/* } */
}

static void fini_lib(CuTest *ct)
{
	nvm_fini();
}

CuSuite *dflash_GetSuite()
{
	per_test_suite = CuSuiteNew();

	SUITE_ADD_TEST(per_test_suite, init_lib);
	SUITE_ADD_TEST(per_test_suite, create_tgt);
	SUITE_ADD_TEST(per_test_suite, remove_tgt);
	SUITE_ADD_TEST(per_test_suite, create_file);
	SUITE_ADD_TEST(per_test_suite, append_file);
	SUITE_ADD_TEST(per_test_suite, fini_lib);
}

void run_all_test(void)
{
	CuString *output = CuStringNew();
	CuSuite *suite = CuSuiteNew();

	CuSuiteAddSuite(suite, (CuSuite*) dflash_GetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);

	CuStringDelete(output);
	CuSuiteDelete(suite);

	free(per_test_suite);
}

int main(void)
{
	run_all_test();
	return 0;
}
