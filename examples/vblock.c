#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_vblock(const char* tgt_name)
{
	NVM_VBLOCK vblock;
	NVM_TGT tgt;
	int ret;

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

	/* Do something with the block e.g. print/read/write */
	nvm_vblock_pr(vblock);

	ret = nvm_vblock_put(vblock, tgt);	/* Release vblock from tgt */
	if (ret) {
		printf("Failed putting block via tgt(%p)\n", tgt);
	}

	nvm_vblock_free(&vblock);		/* De-allocate vblock */
	nvm_tgt_close(tgt);			/* Close the target */
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s target_name e.g. tgt0\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	ex_vblock(argv[1]);

	return 0;
}
