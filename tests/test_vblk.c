#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>

#include <CUnit/Basic.h>

// Parsed from CLI
static char nvm_dev_path[NVM_DEV_PATH_LEN] = "/dev/nvme0n1";

static int ch_bgn = 0;
static int ch_end = 0;
static int lun_bgn = 0;
static int lun_end = 0;
static int blk = 0;

// Managed by setup/teardown and used by tests
static struct nvm_dev *dev;
static const struct nvm_geo *geo;
size_t nbytes;
struct nvm_vblk *vblk;
char *buf_w, *buf_r;

static int SEED = 1337;

static int VERBOSE = 0;

size_t compare_buffers(char *expected, char *actual, size_t nbytes)
{
	size_t diff = 0;

	for (int i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			++diff;
		}
	}

	return diff;
}

void print_mismatch(char *expected, char *actual, size_t nbytes)
{
	printf("MISMATCHES:\n");
	for (int i = 0; i < nbytes; ++i) {
		if (expected[i] != actual[i]) {
			printf("i(%06d), expected(%c) != actual(%02d|0x%02x|%c)\n",
				i, expected[i], (int)actual[i], (int)actual[i], actual[i]);
		}
	}
}

int setup(void)
{
	srand(SEED);

	dev = nvm_dev_open(nvm_dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		return -1;
	}
	geo = nvm_dev_get_geo(dev);

	vblk = nvm_vblk_alloc_line(dev, ch_bgn, ch_end, lun_bgn, lun_end, blk);
	if (!vblk) {
		perror("nvm_vblk_alloc_line");
		return -1;
	}
	nbytes = nvm_vblk_get_nbytes(vblk);

	buf_w = nvm_buf_alloc(geo, nbytes);
	if (!buf_w) {
		perror("nvm_buf_alloc");
		return -1;
	}
	nvm_buf_fill(buf_w, nbytes);

	// Drop the byte offset into the buffer
	for (size_t offset = 0; offset < nbytes; offset += geo->sector_nbytes) {
		char misc[geo->sector_nbytes];

		sprintf(misc, "-={[%lu]}=-", offset);
		memcpy(buf_w + offset, misc, strlen(misc));
	}

	buf_r = nvm_buf_alloc(geo, nbytes);
	if (!buf_r) {
		perror("nvm_buf_alloc");
		return -1;
	}

	return 0;
}

int teardown(void)
{
	geo = NULL;
	nvm_vblk_free(vblk);
	nvm_dev_close(dev);
	free(buf_r);
	free(buf_w);

	return 0;
}

void _test_VBLK(int test_expected_read_fail)
{
	ssize_t res = 0;

	res = nvm_vblk_erase(vblk);				// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: Erasing vblk");
	}

	if (test_expected_read_fail) {
		res = nvm_vblk_read(vblk, buf_r, nbytes);	// EXPECT: Fail
		CU_ASSERT(res < 0);
		if (res >=  0) {
			CU_FAIL("FAILED: nvm_vblk_read OK of NON-written vblk -> should fail");
			return;
		}
	}
	
	res = nvm_vblk_write(vblk, buf_w, nbytes);		// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: nvm_vblk_write");
		return;
	}

	res = nvm_vblk_read(vblk, buf_r, nbytes);		// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: nvm_vblk_read");
		return;
	}

	CU_ASSERT_NSTRING_EQUAL(buf_w, buf_r, nbytes);
}

void test_VBLK_PE_PW_PR(void)
{
	_test_VBLK(0);
}

void test_VBLK_PE_PR_PW_PR(void)
{
	_test_VBLK(1);
}

size_t rand_offset(size_t nbytes, size_t count, size_t align)
{
	const double var = (double)rand() / (double)RAND_MAX;

	return ((size_t)(var * (nbytes - count)) / align) * align;
}

void test_VBLK_RAND(void)
{
	const size_t read_alignment = geo->nplanes * geo->nsectors * geo->sector_nbytes;
	ssize_t res = 0;

	res = nvm_vblk_erase(vblk);			// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: Erasing vblk");
	}

	res = nvm_vblk_write(vblk, buf_w, nbytes);	// EXPECT: OK
	CU_ASSERT(res >= 0);
	if (res < 0) {
		CU_FAIL("FAILED: nvm_vblk_write");
		return;
	}

	int naddrs = nvm_vblk_get_naddrs(vblk);

	for (int i = 1; i <= naddrs; ++i) {
		size_t count = read_alignment * i;
		size_t nbytes_read = 0;

		char *buf = nvm_buf_alloc(geo, count);
		if (!buf) {
			CU_FAIL("FAILED: nvm_buf_alloc");
			return;
		}

		while (nbytes_read < nbytes) {			// EXPECT: OK
			size_t offset = rand_offset(nbytes, count, read_alignment);

			res = nvm_vblk_pread(vblk, buf, count, offset);
			CU_ASSERT(res >= 0);
			if (res < 0) {
				if (VERBOSE)
					perror("nvm_vblk_pread");
				CU_FAIL("FAILED: nvm_vblk_pread");
				free(buf);
				return;
			}
			CU_ASSERT(res == count);

			if (compare_buffers(buf_w + offset, buf, count)) {
				if (VERBOSE)
					print_mismatch(buf_w + offset, buf, count);
				CU_FAIL("FAILED: buffer mismatch");
				free(buf);
				return;
			}

			nbytes_read += res;
		}

		free(buf);
	}
}

int main(int argc, char **argv)
{
	switch(argc) {
	case 8:
		VERBOSE = atoi(argv[7]);
	case 7:
		blk = atoi(argv[6]);
	case 6:
		lun_end = atoi(argv[5]);
	case 5:
		lun_bgn = atoi(argv[4]);
	case 4:
		ch_end = atoi(argv[3]);
	case 3:
		ch_bgn = atoi(argv[2]);
	case 2:
		if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
			printf("ERR: len(dev_path) > %d characters\n",
			       NVM_DEV_PATH_LEN);
			return 1;
                }
		strncpy(nvm_dev_path, argv[1], NVM_DEV_PATH_LEN);
		break;
	}
	CU_pSuite pSuite = NULL;

	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	pSuite = CU_add_suite("nvm_vblk_*", setup, teardown);
	if (NULL == pSuite) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	if (
	(NULL == CU_add_test(pSuite, "nvm_vblk_RAND", test_VBLK_RAND)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblk_PE_PW_PR", test_VBLK_PE_PW_PR)) ||
	(NULL == CU_add_test(pSuite, "nvm_vblk_PE_PR_PW_PR", test_VBLK_PE_PR_PW_PR)) ||
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
