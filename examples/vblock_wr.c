#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_vblock_wr(const char* dev_name, const char* tgt_name)
{
	NVM_TGT tgt;
	NVM_GEO geo;
	NVM_VBLOCK vblock;
	int i, ret, r_npages, w_npages;

	char *wbuf;
	char *rbuf;

	tgt = nvm_tgt_open(tgt_name, 0x0);	/* Why 0x0? */
	if (!tgt) {
		printf("Failed opening target, does it exist? Create with e.g."
		       "'nvme lnvm -d nvme0n1 -n test_target -t dflash'\n");
		return;
	}

	geo = nvm_dev_get_geo(nvm_tgt_get_dev(tgt));

	ret = posix_memalign((void**)&wbuf, geo.nbytes, geo.io_nbytes_max);
	if (ret) {
		printf("Failed allocating write buffer(%p)\n", wbuf);
		return;
	}
	memset(wbuf, 0, geo.io_nbytes_max);
	strcpy(wbuf, "Hello World of NVM");

	ret = posix_memalign((void**)&rbuf, geo.nbytes, geo.io_nbytes_max);
	if (ret) {
		printf("Failed allocating read buffer(%p)\n", wbuf);
		return;
	}
	memset(rbuf, 0, geo.io_nbytes_max);
	strcpy(rbuf, "I WAS NOT WRITTEN");

	vblock = nvm_vblock_new();		/* Allocate the vblock */
	if (!vblock) {
		printf("Failed allocating vblock\n");
		return;
	}

	/* Reserve via tgt channel 0 lun 0 */
	ret = nvm_vblock_gets(vblock, tgt, 0, 0);
	if (ret) {
		printf("Failed(%d) getting block via tgt(%p)\n", ret, tgt);
		return;
	}
	nvm_vblock_pr(vblock);

	for(i=0; i<geo.npages; i++) {
		w_npages = nvm_vblock_pwrite(vblock, wbuf, 1, i);/* WRITE */
		printf("i(%d), written(%d), wbuf(%s)\n", i, w_npages, wbuf);

		r_npages = nvm_vblock_pread(vblock, rbuf, 1, i);/* READ */
		printf("i(%d), read(%d), rbuf(%s)\n", i, r_npages, rbuf);
	}

	ret = nvm_vblock_put(vblock);	/* Release vblock from tgt */
	if (ret) {
		printf("Failed putting block via tgt(%p)\n", tgt);
	}

	nvm_vblock_free(&vblock);		/* De-allocate vblock */
	nvm_tgt_close(tgt);			/* Close the target */
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s dev_name tgt_name "
		       "e.g. '%s nvme0n1 ex_target'\n", argv[0], argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	ex_vblock_wr(argv[1], argv[2]);

	return 0;
}
