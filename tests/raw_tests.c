/*
 * raw_tests - Tests for raw interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <linux/lightnvm.h>
#include <liblightnvm.h>

#include "CuTest/CuTest.h"

#define N_TEST_BLOCKS 100


/* same structure as used internally in provisioning interface */
struct nvm_fpage {
	uint32_t sec_size;
	uint32_t page_size;
	uint32_t pln_pg_size;
	uint32_t max_sec_io;
};

static CuSuite *per_test_suite = NULL;
static char nvm_dev[DISK_NAME_LEN];
static char nvm_tgt_type[NVM_TTYPE_NAME_MAX] = "dflash";
static char nvm_tgt_name[DISK_NAME_LEN] = "libnvm_test";

static void create_tgt(CuTest *ct)
{
	struct nvm_ioctl_tgt_create c;
	int ret;

	strncpy(c.target.dev, nvm_dev, DISK_NAME_LEN);
	strncpy(c.target.tgttype, nvm_tgt_type, NVM_TTYPE_NAME_MAX);
	strncpy(c.target.tgtname, nvm_tgt_name, DISK_NAME_LEN);
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

	strncpy(r.tgtname, nvm_tgt_name, DISK_NAME_LEN);
	r.flags = 0;

	ret = nvm_remove_target(&r);
	CuAssertIntEquals(ct, 0, ret);
}

static long get_nvblocks(NVM_LUN_STAT *stat)
{
	return stat->nr_free_blocks + stat->nr_inuse_blocks +
							stat->nr_bad_blocks;
}

static int get_dev_info(struct nvm_ioctl_dev_info *dev_info,
						struct nvm_fpage *fpage)
{
	static struct nvm_ioctl_dev_prop *dev_prop;
	int ret = 0;

	ret = nvm_get_device_info(dev_info);
	if (ret)
		goto out;

	dev_prop = &dev_info->prop;

	fpage->sec_size = dev_prop->sec_size;
	fpage->page_size = fpage->sec_size * dev_prop->sec_per_page;
	fpage->pln_pg_size = fpage->page_size * dev_prop->nr_planes;

out:
	return ret;
}

/*
 * Get/Put block without submitting I/Os
 *	- Get block
 *	- Check that a block has moved from free to inuse block list
 *	- Put block
 *	- Check that the block has been returned from inuse to free list
 *	- Repeat N_TEST_BLOCKS times (this does not consume blocks)
 */
static void raw_gp1(CuTest *ct)
{
	NVM_VBLOCK vblocks[N_TEST_BLOCKS];
	NVM_VBLOCK *vblock;
	/* NVM_LUN_STAT lun_status = { */
		/* .nr_free_blocks = 0, */
		/* .nr_inuse_blocks = 0, */
		/* .nr_bad_blocks = 0, */
	/* }; */
	long nvblocks, nvblocks_old;
	int tgt_fd;
	int i;
	int ret = 0;

	create_tgt(ct);
	tgt_fd = nvm_target_open(nvm_tgt_name, 0x0);
	CuAssertTrue(ct, tgt_fd > 0);

	/* Check LUN status */
	// TODO - wait until we have defined this ioctl

	/* get block from lun 0*/
	vblocks[0].vlun_id = 0;

	vblock = &vblocks[0];
	vblock->flags |= NVM_PROV_SPEC_LUN;

	/* get block from lun 0, request LUN satus*/
	ret = nvm_get_block(tgt_fd, 0, vblock);
	CuAssertIntEquals(ct, 0, ret);

	/* nvblocks = get_nvblocks(&lun_status); */

	ret = nvm_put_block(tgt_fd, vblock);

	/* nvblocks_old = nvblocks; */
	/* nvblocks = get_nvblocks(&lun_status); */
	/* CuAssertIntEquals(ct, nvblocks_old, nvblocks); */

	nvm_target_close(tgt_fd);
	remove_tgt(ct);
}

/*
 * Get/Put block and submit I/Os at different granuralities
 *	- Get block
 *	- Check that a block has moved from free to inuse block list
 *	- write/read using pread/pwrite
 *	- Put block
 *	- Check that the block has been returned from inuse to free list
 *	- Repeat N_TEST_BLOCKS times (this does not consume blocks)
 */
static void raw_gp2(CuTest *ct)
{
	NVM_VBLOCK vblocks[N_TEST_BLOCKS];
	NVM_VBLOCK *vblock;
	/* NVM_LUN_STAT lun_status = { */
		/* .nr_free_blocks = 0, */
		/* .nr_inuse_blocks = 0, */
		/* .nr_bad_blocks = 0, */
	/* }; */
	static struct nvm_ioctl_dev_info dev_info;
	static struct nvm_fpage fpage;
	long nvblocks, nvblocks_old;
	char *input_payload;
	char *read_payload, *writer;
	size_t current_ppa;
	unsigned long left_bytes;
	unsigned long i;
	uint32_t pg_sec_ratio;
	int tgt_fd;
	int left_pages;
	int ret = 0;

	create_tgt(ct);
	tgt_fd = nvm_target_open(nvm_tgt_name, 0x0);
	CuAssertTrue(ct, tgt_fd > 0);

	strncpy(dev_info.dev, nvm_dev, DISK_NAME_LEN);
	ret = get_dev_info(&dev_info, &fpage);
	CuAssertIntEquals(ct, 0, ret);

	pg_sec_ratio = fpage.pln_pg_size / fpage.sec_size; //writes
	/* Check LUN status */
	// TODO - wait until we have defined this ioctl

	/* get block from lun 0*/
	vblocks[0].vlun_id = 0;
	vblock = &vblock[0];

	vblock->flags |= NVM_PROV_SPEC_LUN;

	/* get block from lun 0, request LUN satus*/
	ret = nvm_get_block(tgt_fd, 0, vblock);
	CuAssertIntEquals(ct, 0, ret);

	/* nvblocks = get_nvblocks(&lun_status); */

	/* Allocate aligned memory to use direct IO */
	ret = posix_memalign((void**)&input_payload, fpage.sec_size,
							fpage.pln_pg_size);
	if (ret) {
		printf("Could not allocate write aligned memory (%d,%d)\n",
					fpage.sec_size, fpage.pln_pg_size);
		goto clean;
	}

	ret = posix_memalign((void**)&read_payload, fpage.sec_size,
							fpage.pln_pg_size);
	if (ret) {
		printf("Could not allocate read aligned memory(%d,%d)\n",
					fpage.sec_size, fpage.pln_pg_size);
		goto clean;
	}

	for (i = 0; i < fpage.pln_pg_size; i++)
		input_payload[i] = (i % 28) + 65;

	/* write first full write page */
	ret = pwrite(tgt_fd, input_payload, fpage.pln_pg_size,
						vblock->bppa * fpage.sec_size);
	if (ret != fpage.pln_pg_size) {
		printf("Could not write data to vblock\n");
		goto clean;
	}

retry:
	/* read first full write page at once */
	ret = pread(tgt_fd, read_payload, fpage.pln_pg_size,
						vblock->bppa * fpage.sec_size);
	if (ret != fpage.pln_pg_size) {
		if (errno == EINTR)
			goto retry;

		printf("Could not read data to vblock\n");
		goto clean;
	}

	CuAssertByteArrayEquals(ct, input_payload, read_payload,
				fpage.pln_pg_size, fpage.pln_pg_size);

	memset(read_payload, 0, fpage.pln_pg_size);

	left_bytes = fpage.pln_pg_size;
	current_ppa = vblock->bppa;
	writer = read_payload;
	/* read first full write page at 4KB granurality */
	while (left_bytes > 0) {
retry2:
		ret = pread(tgt_fd, writer, fpage.sec_size,
						current_ppa * fpage.sec_size);
		if (ret != fpage.sec_size) {
			if (errno == EINTR)
				goto retry2;

			printf("Could not read data to vblock\n");
			goto clean;
		}

		current_ppa++;
		left_bytes -= fpage.sec_size;
		writer += fpage.sec_size;
	}

	CuAssertByteArrayEquals(ct, input_payload, read_payload,
				fpage.pln_pg_size, fpage.pln_pg_size);

	//JAVIER::: Take npages from kernel
	left_pages = 255;
	current_ppa = vblock->bppa + pg_sec_ratio;

	/* Write rest of block */
	while (left_pages > 0) {
		ret = pwrite(tgt_fd, input_payload, fpage.pln_pg_size,
						current_ppa * fpage.sec_size);
		if (ret != fpage.pln_pg_size) {
			printf("Could not write data to vblock\n");
			goto clean;
		}

		current_ppa += pg_sec_ratio;
		left_pages--;
	}

	left_pages = 255;
	current_ppa = vblock->bppa + pg_sec_ratio;

	/* Read rest of block at full write page granurality */
	while (left_pages > 0) {
retry3:
		memset(read_payload, 0, fpage.pln_pg_size);
		ret = pread(tgt_fd, read_payload, fpage.pln_pg_size,
						current_ppa * fpage.sec_size);
		if (ret != fpage.pln_pg_size) {
			if (errno == EINTR)
				goto retry3;

			printf("Could not read data to vblock\n");
			goto clean;
		}

		CuAssertByteArrayEquals(ct, input_payload, read_payload,
				fpage.pln_pg_size, fpage.pln_pg_size);

		current_ppa += pg_sec_ratio;
		left_pages --;
	}

	left_bytes = fpage.pln_pg_size * 255;
	current_ppa = vblock->bppa + pg_sec_ratio;
	writer = read_payload;

	int cnt = 0;
	/* Read rest of block at 4KB granurality */
	while (left_bytes > 0) {
retry4:
		ret = pread(tgt_fd, writer, fpage.sec_size,
						current_ppa * fpage.sec_size);
		if (ret != fpage.sec_size) {
			if (errno == EINTR)
				goto retry4;

			printf("Could not read data to vblock\n");
			goto clean;
		}

		current_ppa++;
		left_bytes -= fpage.sec_size;
		cnt++;

		if (cnt == pg_sec_ratio) {
			CuAssertByteArrayEquals(ct, input_payload, read_payload,
				fpage.pln_pg_size, fpage.pln_pg_size);
			memset(read_payload, 0, fpage.pln_pg_size);
			writer = read_payload;
			cnt = 0;
		} else
			writer += fpage.sec_size;
	}

clean:
	free(input_payload);
	free(read_payload);

	ret = nvm_put_block(tgt_fd, vblock);

	/* nvblocks_old = nvblocks; */
	/* nvblocks = get_nvblocks(&lun_status); */
	/* CuAssertIntEquals(ct, nvblocks_old, nvblocks); */

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
	SUITE_ADD_TEST(per_test_suite, raw_gp2);
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
		printf("Usage: %s nvm_dev / nvm_dev: LightNVM device\n",
									argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("Argument nvm_dev can be maximum %d characters\n",
							DISK_NAME_LEN - 1);
	}

	strcpy(nvm_dev, argv[1]);

	run_all_test();

	return 0;
}
