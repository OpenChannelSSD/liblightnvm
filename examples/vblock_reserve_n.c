#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_vblock_reserve_n(const char* dev_name, const char* tgt_name,
			 int nvblocks)
{
	NVM_VBLOCK *vblocks;
	NVM_TGT tgt;

	int ret, i;

	tgt = nvm_tgt_open(tgt_name, 0x0);		/* Open target */
	if (!tgt) {
		printf("Failed opening target, does it exist? Create with e.g."
		       "'nvme lnvm -d nvme0n1 -n test_target -t dflash'\n");
		return;
	}

	vblocks = malloc(sizeof(*vblocks) * nvblocks);	/* Allocate vblocks */
	if (!vblocks) {
		printf("Failed allocating array of vblocks\n");
		return;
	}
	for(i=0; i<nvblocks; ++i) {
		vblocks[i] = nvm_vblock_new();
		if (!vblocks[i]) {
			printf("Failed allocating vblock(%d)\n", i);
			return;
		}
	}

	// NOTE: Reserves round-robin across lun 0-3
	for(i=0; i<nvblocks; ++i) {			/* Reserve vblocks */
		printf("_gets #%d : ", i);
		ret = nvm_vblock_gets(vblocks[i], tgt, i % 4);
		if (ret) {
			printf("failed(%d) via tgt(%p)\n", ret, tgt);
		} else {
			nvm_vblock_pr(vblocks[i]);
		}
	}

	for(i=0; i<nvblocks; ++i) {			/* Release blocks */
		printf("_put #%d : ", i);
		ret = nvm_vblock_put(vblocks[i], tgt);
		if (ret) {
			printf("failed(%d) via tgt(%p)\n", ret, tgt);
		} else {
			printf("OK\n");
		}
	}

	for(i=0; i<nvblocks; ++i) {			/* Deallocate vblocks */
		nvm_vblock_free(&vblocks[i]);
	}
	free(vblocks);

	nvm_tgt_close(tgt);				/* Close the target */
}

int main(int argc, char **argv)
{
	if (argc != 4) {
		printf("Usage: %s dev_name tgt_name num_vblocks"
		       "e.g. '%s nvme0n1 ex_target'\n", argv[0], argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	ex_vblock_reserve_n(argv[1], argv[2], atoi(argv[3]));

	return 0;
}
