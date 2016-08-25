#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_vblock_rw(const char* dev_name, const char* tgt_name)
{
	NVM_DEV dev;
	uint32_t sec_size, pln_pg_size;

	NVM_TGT tgt;
	NVM_VBLOCK vblock;
	int ret, read, written;

	char *wbuf;
	char *rbuf;

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device(%s)\n", dev_name);
		return;
	}

	sec_size = nvm_dev_get_sec_size(dev);
	pln_pg_size = nvm_dev_get_pln_pg_size(dev);

	tgt = nvm_tgt_open(tgt_name, 0x0);	/* Why 0x0? */
	if (!tgt) {
		printf("Failed opening target, does it exist? Create with e.g."
		       "'nvme lnvm -d nvme0n1 -n test_target -t dflash'\n");
		return;
	}

	vblock = nvm_vblock_new();		/* Allocate the vblock */
	if (!vblock) {
		printf("Failed allocating vblock\n");
		return;
	}

	ret = nvm_vblock_gets(vblock, tgt, 0, 0);	/* Reserve from tgt on lun 0 */
	if (ret) {
		printf("Failed getting block via tgt(%p)\n", tgt);
		return;
	}
						/* Write to media */
	ret = posix_memalign((void**)&wbuf, sec_size, pln_pg_size);
	if (ret) {
		printf("Failed allocating write buffer(%p)\n", wbuf);
		return;
	}
	strcpy(wbuf, "Hello World of NVM");
	written = nvm_vblock_write(vblock, wbuf, 1, 0, 0x0);
	printf("written(%d)\n", written);

						/* Read from media */
	ret = posix_memalign((void**)&rbuf, sec_size, pln_pg_size);
	if (ret) {
		printf("Failed allocating read buffer(%p)\n", wbuf);
		return;
	}
	read = nvm_vblock_read(vblock, rbuf, 1, 0, 0x0);
	printf("read(%d), rbuf(%s)\n", read, rbuf);

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

	ex_vblock_rw(argv[1], argv[2]);

	return 0;
}
