/*
 * raw_tests - Tests for raw interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

/*
 * These tests require access to data behind the scenes since it is allocating
 * memory for NVM_VBLOCK and manipulating properties of NVM_VBLOCK.
 */

static char nvm_dev_name[DISK_NAME_LEN] = "nvme0n1";
static char nvm_tgt_type[NVM_TTYPE_NAME_MAX] = "dflash";
static char nvm_tgt_name[DISK_NAME_LEN] = "nvm_vblock_tst";

int init_suite1(void)
{
	return 0;
}

int clean_suite1(void)
{
	return 0;
}

void test_VBLOCK_NEW_FREE(void)
{
	NVM_VBLOCK vblock;

	vblock = nvm_vblock_new();
	CU_ASSERT_PTR_NOT_NULL(vblock);

	nvm_vblock_free(&vblock);
	CU_ASSERT_PTR_NULL(vblock);
}

/*
 * Get/Put block without submitting I/Os
 *	- Get block
 *	- Check that a block has moved from free to inuse block list
 *	- Put block
 *	- Check that the block has been returned from inuse to free list
 *	- Repeat N_TEST_BLOCKS times (this does not consume blocks)
 */
void test_VBLOCK_GET_PUT_01(void)
{
	NVM_VBLOCK vblock;
	NVM_TGT tgt;
	int ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev_name, 0, 0);
	CU_ASSERT(0==ret);

	tgt = nvm_tgt_open(nvm_tgt_name, 0x0);
	CU_ASSERT(tgt > 0);

        /* get block from lun 0*/
	vblock = nvm_vblock_new();
	CU_ASSERT_PTR_NOT_NULL(vblock);

	/* get block from lun 0 */
	ret = nvm_vblock_get(vblock, tgt);
	CU_ASSERT(0==ret);

	ret = nvm_vblock_put(vblock, tgt);
	CU_ASSERT(0==ret);

	nvm_vblock_free(&vblock);
	CU_ASSERT_PTR_NULL(vblock);

	nvm_tgt_close(tgt);
	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0==ret);
}

/*
 * Get/Put block without submitting I/Os
 *	- Get block
 *	- Check that a block has moved from free to inuse block list
 *	- Put block
 *	- Check that the block has been returned from inuse to free list
 *	- Repeat N_TEST_BLOCKS times (this does not consume blocks)
 */
void test_VBLOCK_GETS_PUT_01(void)
{
	NVM_VBLOCK vblock;
	NVM_TGT tgt;
	int ret;

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev_name, 0, 0);
	CU_ASSERT(0==ret);

	tgt = nvm_tgt_open(nvm_tgt_name, 0x0);
	CU_ASSERT_PTR_NOT_NULL(tgt > 0);

	/* get block from lun 0*/
	vblock = nvm_vblock_new();
	CU_ASSERT_PTR_NOT_NULL(vblock);

	/* get block from lun 0, request LUN satus*/
	ret = nvm_vblock_gets(vblock, tgt, 0);
	CU_ASSERT(0==ret);

	ret = nvm_vblock_put(vblock, tgt);

	nvm_tgt_close(tgt);
	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0==ret);
}

/*
 * Get/Put block and submit I/Os at different granuralities
 *	- Get block
 *	- Check that a block has moved from free to inuse block list
 *	- write/read using pread/pwrite
 *	- Put block
 *	- Check that the block has been returned from inuse to free list
 */
void test_VBLOCK_GETS_PUT_02(void)
{
	NVM_DEV dev;
	NVM_TGT tgt;
	NVM_VBLOCK vblock;
	NVM_FPAGE fpage;
	int tgt_fd;

	char *input_payload = NULL;
	char *read_payload = NULL;
	char *writer;
	size_t current_ppa;
	unsigned long left_bytes;
	int left_pages;
	int ret = 0;
	int i;

	uint32_t sec_size, pln_pg_size;
	uint32_t pg_sec_ratio;

	dev = nvm_dev_open(nvm_dev_name);
	CU_ASSERT_PTR_NOT_NULL_FATAL(dev);

	fpage = nvm_dev_get_fpage(dev);
	CU_ASSERT_PTR_NOT_NULL(fpage);

	sec_size = nvm_fpage_get_sec_size(fpage);
	pln_pg_size = nvm_fpage_get_pln_pg_size(fpage);
	pg_sec_ratio = pln_pg_size / sec_size; //writes

	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev_name, 0, 0);
	CU_ASSERT(0==ret);

	tgt = nvm_tgt_open(nvm_tgt_name, 0x0);
	CU_ASSERT_PTR_NOT_NULL(tgt);

	tgt_fd = nvm_tgt_get_fd(tgt);

	/* Check LUN status */
	// TODO - wait until we have defined this ioctl

	vblock = nvm_vblock_new();
	CU_ASSERT_PTR_NOT_NULL(vblock);

	ret = nvm_vblock_gets(vblock, tgt, 0);
	CU_ASSERT(0 == ret);

	/* Allocate aligned memory to use direct IO */
	ret = posix_memalign((void**)&input_payload, sec_size, pln_pg_size);
	if (ret) {
		printf("raw_gp2: Failed allocating write aligned (%d,%d)\n",
			sec_size, pln_pg_size);
		goto clean;
	}

	ret = posix_memalign((void**)&read_payload, sec_size, pln_pg_size);
	if (ret) {
		printf("raw_gp2: Failed allocating read aligned(%d,%d)\n",
		       sec_size, pln_pg_size);
		goto clean;
	}

	for (i = 0; i < pln_pg_size; i++)
		input_payload[i] = (i % 28) + 65;

	/* write first full write page */
	ret = pwrite(tgt_fd, input_payload, pln_pg_size,
                     nvm_vblock_get_bppa(vblock) * sec_size);
	if (ret != pln_pg_size) {
		printf("raw_gp2: Could not write data to vblock\n");
		goto clean;
	}

retry:
	/* read first full write page at once */
	ret = pread(tgt_fd, read_payload, pln_pg_size,
                    nvm_vblock_get_bppa(vblock) * sec_size);
	if (ret != pln_pg_size) {
		if (errno == EINTR)
			goto retry;

		printf("raw_gp2: Could not read data to vblock\n");
		goto clean;
	}

	// TODO: Check strings
	//CuAssertByteArrayEquals(ct, input_payload, read_payload,
	//			fpage.pln_pg_size, fpage.pln_pg_size);
	CU_ASSERT_STRING_EQUAL(input_payload, read_payload);

	memset(read_payload, 0, pln_pg_size);

	left_bytes = pln_pg_size;
	current_ppa = nvm_vblock_get_bppa(vblock);
	writer = read_payload;
	/* read first full write page at 4KB granurality */
	while (left_bytes > 0) {
retry2:
		ret = pread(tgt_fd, writer, sec_size, current_ppa * sec_size);
		if (ret != sec_size) {
			if (errno == EINTR)
				goto retry2;

			printf("raw_gp2: Could not read data to vblock\n");
			goto clean;
		}

		current_ppa++;
		left_bytes -= sec_size;
		writer += sec_size;
	}

	// TODO: Check strings
	//CuAssertByteArrayEquals(ct, input_payload, read_payload,
	//			fpage.pln_pg_size, fpage.pln_pg_size);
	CU_ASSERT_STRING_EQUAL(input_payload, read_payload);

	//JAVIER::: Take npages from kernel
	left_pages = 255;
	current_ppa = nvm_vblock_get_bppa(vblock) + pg_sec_ratio;

	/* Write rest of block */
	while (left_pages > 0) {
		int written = pwrite(tgt_fd, input_payload, pln_pg_size,
			     current_ppa * sec_size);
		if (written != pln_pg_size) {
			printf("raw_gp2: Could not write data to vblock\n");
			goto clean;
		}

		current_ppa += pg_sec_ratio;
		left_pages--;
	}

	left_pages = 255;
	current_ppa = nvm_vblock_get_bppa(vblock) + pg_sec_ratio;

	/* Read rest of block at full write page granurality */
	while (left_pages > 0) {
retry3:
		memset(read_payload, 0, pln_pg_size);
		ret = pread(tgt_fd, read_payload, pln_pg_size,
						current_ppa * sec_size);
		if (ret != pln_pg_size) {
			if (errno == EINTR)
				goto retry3;

			printf("raw_gp2: Could not read data to vblock\n");
			goto clean;
		}

		// TODO: Check strings
		//CuAssertByteArrayEquals(ct, input_payload, read_payload,
		//		fpage.pln_pg_size, fpage.pln_pg_size);
		CU_ASSERT_STRING_EQUAL(input_payload, read_payload);

		current_ppa += pg_sec_ratio;
		left_pages --;
	}

	left_bytes = pln_pg_size * 255;
	current_ppa = nvm_vblock_get_bppa(vblock) + pg_sec_ratio;
	writer = read_payload;

	int cnt = 0;
	/* Read rest of block at 4KB granurality */
	while (left_bytes > 0) {
retry4:
		ret = pread(tgt_fd, writer, sec_size,
						current_ppa * sec_size);
		if (ret != sec_size) {
			if (errno == EINTR)
				goto retry4;

			printf("raw_gp2: Could not read data to vblock\n");
			goto clean;
		}

		current_ppa++;
		left_bytes -= sec_size;
		cnt++;

		if (cnt == pg_sec_ratio) {
			// TODO: Check strings are equal
			//CuAssertByteArrayEquals(ct, input_payload, read_payload,
			//	fpage.pln_pg_size, fpage.pln_pg_size);
			CU_ASSERT_STRING_EQUAL(input_payload, read_payload);
			memset(read_payload, 0, pln_pg_size);
			writer = read_payload;
			cnt = 0;
		} else
			writer += sec_size;
	}

clean:
	free(input_payload);
	free(read_payload);

	ret = nvm_vblock_put(vblock, tgt);

	nvm_tgt_close(tgt);
	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0==ret);
}

void __test_VBLOCK_WRITE_READ_N(int iterations, int npage_io)
{
	NVM_DEV dev;
	NVM_FPAGE fpage;
	uint32_t sec_size, pln_pg_size;

	NVM_TGT tgt;
	NVM_VBLOCK vblock;
	int i, j, ret;
						/* Create target */
	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev_name, 0, 0);
	CU_ASSERT(0==ret);

	dev = nvm_dev_open(nvm_dev_name);		/* Open device */
	CU_ASSERT_PTR_NOT_NULL(dev);

	fpage = nvm_dev_get_fpage(dev);		/* Get geometry */
	sec_size = nvm_fpage_get_sec_size(fpage);
	pln_pg_size = nvm_fpage_get_pln_pg_size(fpage);

	tgt = nvm_tgt_open(nvm_tgt_name, 0x0);	/* Open target */
	CU_ASSERT_PTR_NOT_NULL(tgt);

	for(i=0; i<iterations; ++i) {
		int read, written;
		char *wbuf = NULL;
		char *rbuf = NULL;
							/* Allocate buffers */
		ret = posix_memalign((void**)&wbuf, sec_size, pln_pg_size);
		CU_ASSERT(0==ret)
		strcpy(wbuf, "Hello World of NVM");

		ret = posix_memalign((void**)&rbuf, sec_size, pln_pg_size);
		CU_ASSERT(0==ret);

		vblock = nvm_vblock_new();		/* Allocate vblock */
		CU_ASSERT_PTR_NOT_NULL(vblock);

		ret = nvm_vblock_gets(vblock, tgt, 0);	/* Reserve from tgt on lun 0 */
		CU_ASSERT(0==ret)
		for(j=0; j<npage_io; ++j) {
			written = nvm_vblock_write(vblock, tgt, wbuf, 1, 0, 0x0);
			CU_ASSERT(1==written)

			read = nvm_vblock_read(vblock, tgt, rbuf, 1, 0, 0x0);
			CU_ASSERT(1==read);

			CU_ASSERT_STRING_EQUAL(wbuf, rbuf);
		}

		ret = nvm_vblock_put(vblock, tgt);	/* Release vblock from tgt */
		CU_ASSERT(0==ret);

		free(wbuf);
		free(rbuf);

		nvm_vblock_free(&vblock);		/* De-allocate vblock */
	}

	nvm_tgt_close(tgt);				/* Close the target */

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(0==ret);
}

void test_VBLOCK_WRITE_READ_01(void)
{
	__test_VBLOCK_WRITE_READ_N(1, 1);
}

void test_VBLOCK_WRITE_READ_02(void)
{
	__test_VBLOCK_WRITE_READ_N(1000, 1);
}

void test_VBLOCK_WRITE_READ_03(void)
{
	__test_VBLOCK_WRITE_READ_N(1, 1000);
}

int main(int argc, char **argv)
{
	if (argc > 1) {
                if (strlen(argv[1]) > DISK_NAME_LEN) {
                        printf("Argument nvm_dev can be maximum %d characters\n",
                                                                DISK_NAME_LEN - 1);
                }
		strcpy(nvm_dev_name, argv[1]);
	}

	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_vblock*", init_suite1, clean_suite1);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_vblock_[new|free]", test_VBLOCK_NEW_FREE)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[get|put] 1", test_VBLOCK_GET_PUT_01)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[gets|put] 1", test_VBLOCK_GETS_PUT_01)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[gets|put] 2", test_VBLOCK_GETS_PUT_02)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 1", test_VBLOCK_WRITE_READ_01)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 2", test_VBLOCK_WRITE_READ_02)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblock_[write|read] 3", test_VBLOCK_WRITE_READ_03)) ||
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
