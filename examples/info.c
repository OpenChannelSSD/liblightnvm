/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s /dev/dev_name\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
		printf("len(dev_path) > %d\n", NVM_DEV_PATH_LEN - 1);
	}

	NVM_DEV dev = nvm_dev_open(argv[1]);
	if (!dev) {
		printf("Failed opening device\n");
		return -1;
	}

	printf("** Device information  -- nvm_dev_pr **\n");
	nvm_dev_pr(dev);

	nvm_dev_close(dev);

	return 0;
}
