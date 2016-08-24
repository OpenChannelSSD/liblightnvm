#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_vblock_rw(const char* dev_name, const char* tgt_name)
{
	NVM_DEV dev;
	NVM_FPAGE fpage;
	uint32_t sec_size, pln_pg_size;

	NVM_TGT tgt;
	NVM_VBLOCK vblocks[4];
	int nvblocks = 4;
	int i, ret, read, written;

	char *wbuf;
	char *rbuf;

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device(%s)\n", dev_name);
		return;
	}

	fpage = nvm_dev_get_fpage(dev);
	sec_size = nvm_fpage_get_sec_size(fpage);
	pln_pg_size = nvm_fpage_get_pln_pg_size(fpage);
	nvm_fpage_pr(fpage);

	ret = posix_memalign((void**)&wbuf, sec_size, pln_pg_size);
	if (ret) {
		printf("Failed allocating write buffer(%p)\n", wbuf);
		return;
	}
	strcpy(wbuf, "Hello World of NVM");

	ret = posix_memalign((void**)&rbuf, sec_size, pln_pg_size);
	if (ret) {
		printf("Failed allocating read buffer(%p)\n", wbuf);
		return;
	}

	tgt = nvm_tgt_open(tgt_name, 0x0);
	if (!tgt) {
		printf("Failed opening target, does it exist? Create with e.g."
		       "'nvme lnvm -d nvme0n1 -n test_target -t dflash'\n");
		return;
	}

	for(i=0; i<nvblocks; ++i) {
		vblocks[i] = nvm_vblock_new();		/* Allocate the vblock */
		if (!vblocks[i]) {
			printf("Failed allocating vblocks[%d]\n", i);
			return;
		}
	}

	for(i=0; i<nvblocks; ++i) {	/* Reserve from tgt on lun 0-3 */
		ret = nvm_vblock_gets(vblocks[i], tgt, i);
		if (ret) {
			printf("Failed getting block via tgt(%p)\n", tgt);
			return;
		}
		nvm_vblock_pr(vblocks[i]);
	}

	for(i=0; i<nvblocks; ++i) {
							/* Write to media */
		written = nvm_vblock_write(vblocks[i], tgt, wbuf, 1, 0, 0x0);
		printf("written(%d)\n", written);
							
		strcpy(rbuf, "");			/* Read from media */
		read = nvm_vblock_read(vblocks[i], tgt, rbuf, 1, 0, 0x0);
		printf("read(%d), rbuf(%s)\n", read, rbuf);
	}

	for(i=0; i<nvblocks; ++i) {
		ret = nvm_vblock_put(vblocks[i], tgt);	/* Release vblock from tgt */
		if (ret) {
			printf("Failed putting block via tgt(%p)\n", tgt);
		}
	}

	for(i=0; i<nvblocks; ++i) {
		nvm_vblock_free(&vblocks[i]);		/* De-allocate vblock */
	}
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

	ex_vblock_rw(argv[1], argv[2]);

	return 0;
}
