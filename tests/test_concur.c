#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <liblightnvm.h>
#include <pthread.h>

#include <CUnit/Basic.h>

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

struct context {
	NVM_VBLOCK blk;
	NVM_TGT tgt;
	char *buf;
};

#define NUM_PAGES (64)
static void *write_thread(void *priv)
{
	struct context *ctx = priv;
	int i, w_nbytes;

	for (i = 0; i < NUM_PAGES; i++) {
		w_nbytes = nvm_vblock_write(ctx->blk, ctx->buf, 1, i);/* WRITE */
		printf("i(%d), written(%d), wbuf(%s)\n", i, w_nbytes, ctx->buf);
	}
	pthread_exit(NULL);
}

static void *erase_thread(void *ctx)
{

	pthread_exit(NULL);
}

#define NUM_BLOCKS (2)
#define WR_BUF (4096 * 4 * 1024)

void test_VBLOCK_CONCUR(void)
{
	NVM_VBLOCK vblock[2];
	NVM_TGT tgt;
	int ret, i, r_nbytes;
	struct context ctx[2];
	char *wbuf;
	pthread_t wr_th, er_th;

	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	ret = nvm_mgmt_tgt_create(nvm_tgt_name, nvm_tgt_type, nvm_dev_name, 0, 0);
	CU_ASSERT(!ret);

	tgt = nvm_tgt_open(nvm_tgt_name, 0x0);
	CU_ASSERT(tgt > 0);

	for (i = 0; i < NUM_BLOCKS; i++) {
		vblock[i] = nvm_vblock_new();
		ret = nvm_vblock_gets(vblock[i], tgt, 0, 0);
		CU_ASSERT(!ret);
		nvm_vblock_pr(vblock[i]);
	}

	ret = posix_memalign((void**)&wbuf, 4096, WR_BUF);
	if (ret) {
		printf("Failed allocating write buffer(%p)\n", wbuf);
		return;
	}
	memset(wbuf, 0, WR_BUF);

	ctx[0].blk = vblock[0];
	ctx[0].tgt = tgt;
	ctx[0].buf = wbuf;

	ctx[1].blk = vblock[1];
	ctx[1].tgt = tgt;
	ctx[1].buf = wbuf;

	if (pthread_create(&wr_th, NULL, write_thread, &ctx[0])) {
		fprintf(stderr, "fail...\n");
		return;
	}

	if (pthread_create(&er_th, NULL, erase_thread, &ctx[1])) {
		fprintf(stderr, "fail2...\n");
		return;
	}

	pthread_join(wr_th, NULL);
	pthread_join(er_th, NULL);

	for (i = 0; i < NUM_PAGES; i++) {
		r_nbytes = nvm_vblock_read(vblock[0], wbuf, 1, i);	/* READ */
		printf("i(%d), read(%d), rbuf(%s)\n", i, r_nbytes, wbuf);
	}

	for (i = 0; i < NUM_BLOCKS; i++) {
		ret = nvm_vblock_put(vblock[i]);
		CU_ASSERT(!ret);

		nvm_vblock_free(&vblock[i]);
	}

	nvm_tgt_close(tgt);
	ret = nvm_mgmt_tgt_remove(nvm_tgt_name);
	CU_ASSERT(!ret);
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
	(NULL == CU_add_test(pSuite, "nvm_concur_write-erase", test_VBLOCK_CONCUR)) ||
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
