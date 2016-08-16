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

	NVM_TGT_INFO info = nvm_tgt_info_new();
	nvm_tgt_info_fill(info, argv[1]);
	nvm_tgt_info_pr(info);
	nvm_tgt_info_free(&info);

	return 0;
}
