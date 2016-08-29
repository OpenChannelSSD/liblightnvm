/* Target info example */
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s tgt_name e.g. tgt0\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	NVM_TGT tgt = nvm_tgt_open(argv[1], 0x0);

	nvm_tgt_pr(tgt);

	nvm_dev_pr(nvm_tgt_get_dev(tgt));

	nvm_geo_pr(nvm_dev_get_geo(nvm_tgt_get_dev(tgt)));

	return 0;
}
