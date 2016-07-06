#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

#define BUF_LEN 4096

// WIP: Example is broken...

void ex_vblock_rw(const char* dev_name, const char* tgt_name)
{
	NVM_DEV dev;
	NVM_FPAGE fpage;
	NVM_TGT tgt;
	NVM_VBLOCK vblock;
	int ret, read, written;

	char wbuf[BUF_LEN] = "Hello";
	char rbuf[BUF_LEN] = "";

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device(%s)\n", dev_name);
		return;
	}

	fpage = nvm_dev_get_fpage(dev);

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

	ret = nvm_vblock_gets(vblock, tgt, 0);	/* Reserve from tgt on lun 0 */
	if (ret) {
		printf("Failed getting block via tgt(%p)\n", tgt);
		return;
	}

	nvm_vblock_pr(vblock);
	written = nvm_vblock_write(vblock, fpage, 1, 1, tgt, wbuf, 0x0);
	printf("written(%d)\n", written);

	read = nvm_vblock_read(vblock, fpage, 1, 1, tgt, rbuf, 0x0);
	printf("read(%d), rbuf(%s)\n", read, rbuf);

	ret = nvm_vblock_put(vblock, tgt);	/* Release vblock from tgt */
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
