/* Target info example */
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s dev_name\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	NVM_DEV dev = nvm_dev_open(argv[1]);
	if (!dev) {
		printf("Failed opening device\n");
		return -1;
	}

	printf("** Device information  -- nvm_dev_pr **\n");
	nvm_dev_pr(dev);

	printf("** Device geometry -- nvm_geo_pr **\n");
	nvm_geo_pr(nvm_dev_get_geo(dev));

	nvm_dev_close(dev);

	return 0;
}
