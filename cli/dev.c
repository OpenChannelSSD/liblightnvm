#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <liblightnvm.h>

#define CLI_CMD_LEN 50

int info(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addrs[], int naddrs, int flags)
{
	printf("** Device information  -- nvm_dev_pr **\n");
	nvm_dev_pr(dev);

	return 0;
}

typedef struct {
	char name[CLI_CMD_LEN];
	int (*func)(NVM_DEV, NVM_GEO, NVM_ADDR[], int, int);
	int argc;
	int flags;
} NVM_CLI_DEV_CMD;

static NVM_CLI_DEV_CMD cmds[] = {
	{"info", info, -1, 0x0},
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);

void _usage_pr(char *cli_name)
{
	int i;
	
	printf("Usage:\n");
	for (i = 0; i < ncmds; ++i) {
		printf(" %s /dev/dev_name\n", cli_name);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		_usage_pr(argv[0]);
		return EINVAL;
	}

	if (strlen(argv[1]) > NVM_DEV_PATH_LEN) {
		_usage_pr(argv[0]);
		return EINVAL;
	}

	NVM_DEV dev = nvm_dev_open(argv[1]);
	if (!dev) {
		perror("nvm_dev_open");
		return EINVAL;
	}

	nvm_dev_close(dev);

	return 0;
}
