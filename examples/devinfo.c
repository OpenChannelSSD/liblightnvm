/* Device info example */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s device_name e.g. nvme0n1\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	NVM_DEV_INFO info = nvm_dev_info_new();
	nvm_dev_info_fill(info, argv[1]);
	nvm_dev_info_pr(info);
	nvm_dev_info_free(&info);

	return 0;
}
