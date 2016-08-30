/* Target info example */
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s tgt_name\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	NVM_TGT tgt = nvm_tgt_open(argv[1], 0x0);
	NVM_DEV dev = nvm_tgt_get_dev(tgt);
	NVM_GEO geo = nvm_dev_get_geo(dev);

	printf("** Target information **\n");
	nvm_tgt_pr(tgt);

	printf("** Associated device information **\n");
	nvm_dev_pr(dev);

	printf("** Associated device geometry **\n");
	nvm_geo_pr(geo);

	return 0;
}
