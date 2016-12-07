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
	{"info", info, 3, 0x0},
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);

void _usage_pr(char *cli_name)
{
	int i;
	
	printf("Usage:\n");
	for (i = 0; i < ncmds; ++i) {
		printf(" %s %s dev_path\n", cli_name, cmds[i].name);
	}
}

int main(int argc, char **argv)
{
	char cmd_name[CLI_CMD_LEN];
	char dev_path[NVM_DEV_PATH_LEN+1];
	int ret, i;

	NVM_DEV dev;
	NVM_GEO geo;

	NVM_CLI_DEV_CMD *cmd = NULL;

	if (argc < 3) {
		_usage_pr(argv[0]);
		return -EINVAL;
	}
							// Get `cmd_name`
	if (strlen(argv[1]) < 1 || strlen(argv[1]) > (CLI_CMD_LEN-1)) {
		printf("Invalid cmd\n");
		_usage_pr(argv[0]);
		return EINVAL;
	}
	memset(cmd_name, 0, sizeof(cmd_name));
	strcpy(cmd_name, argv[1]);

	for (i = 0; i < ncmds; ++i) {			// Get `cmd`
		if (strcmp(cmd_name, cmds[i].name) == 0) {
			cmd = &cmds[i];
			break;
		}
	}
	if (!cmd) {
		printf("Invalid cmd(%s)\n", cmd_name);
		_usage_pr(argv[0]);
		return EINVAL;
	}

	if (argc != cmd->argc) {			// Check argument count
		printf("Invalid cmd(%s) argc(%d) != %d\n",
			cmd_name, argc, cmd->argc);
		_usage_pr(argv[0]);
		return 1;
	}

	if (strlen(argv[2]) > NVM_DEV_PATH_LEN) {	// Get `dev_path`
		printf("len(dev_path) > %d\n", NVM_DEV_PATH_LEN);
		return 1;
	}
	strncpy(dev_path, argv[2], NVM_DEV_PATH_LEN);

	dev = nvm_dev_open(dev_path);
	if (!dev) {
		perror("nvm_dev_open");
		return EINVAL;
	}

	geo = nvm_dev_attr_geo(dev);

	ret = cmd->func(dev, geo, NULL, 0, cmd->flags);
	if (ret)
		perror("Command failed");

	nvm_dev_close(dev);

	return 0;
}
