/**
 * bbt - CLI example for bad-block-table
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int get(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_BBT* bbt;
	NVM_RET ret = {0, 0};

	printf("** nvm_bbt_get(...):\n");

	bbt = nvm_bbt_get(dev, addr, &ret);
	if (!bbt) {
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		return 1;
	}

	nvm_bbt_pr(bbt);

	free(bbt->blks);
	free(bbt);

	return 0;
}

int set(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	return 0;
}

/* The rest is CLI boilerplate */

#define CLI_CMD_LEN 50

typedef struct {
	char name[CLI_CMD_LEN];
	int (*func)(NVM_DEV dev, NVM_GEO geo, NVM_ADDR, int);
	int argc;
	int flags;
} NVM_CLI_BBT_CMD;

static NVM_CLI_BBT_CMD cmds[] = {
	{"get", get, 5, 0x0},
	{"set", set, 5, 0x0},
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);
static char *args[] = {
	"dev_path",
	"ch",
	"lun",
	"blk"
};

void _usage_pr(char *cli_name)
{
	int cmd;

	printf("Usage:\n");
	for (cmd = 0; cmd < ncmds; cmd++) {
		int arg;
		printf(" %s %6s", cli_name, cmds[cmd].name);
		for (arg = 0; arg < cmds[cmd].argc-2; ++arg) {
			printf(" %s", args[arg]);
		}
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	char cmd_name[CLI_CMD_LEN];
	char dev_path[NVM_DEV_PATH_LEN+1];
	int i, bounds, ret = 0;

	NVM_CLI_BBT_CMD *cmd = NULL;
	
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_ADDR addr;

	if (argc < 3) {
		_usage_pr(argv[0]);
		return -1;
	}
							// Get `cmd_name`
	if (strlen(argv[1]) < 1 || strlen(argv[1]) > (CLI_CMD_LEN-1)) {
		printf("Invalid cmd\n");
		_usage_pr(argv[0]);
		return -EINVAL;
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
		return -EINVAL;
	}

	if (argc != cmd->argc) {			// Check argument count
		printf("Invalid cmd(%s) argc(%d) != %d\n",
			cmd_name, argc, cmd->argc);
		_usage_pr(argv[0]);
		return -1;
	}

	if (strlen(argv[2]) > NVM_DEV_PATH_LEN) {	// Get `dev_path`
		printf("len(dev_path) > %d\n", NVM_DEV_PATH_LEN);
		return -1;
	}
	strncpy(dev_path, argv[2], NVM_DEV_PATH_LEN);

	dev = nvm_dev_open(dev_path);			// open `dev`
	if (!dev) {
		printf("FAILED: opening device, dev_path(%s)\n", dev_path);
		return -EINVAL;
	}

	geo = nvm_dev_attr_geo(dev);

	addr.ppa = 0;
	addr.g.ch = atol(argv[3]);
	addr.g.lun = atol(argv[4]);

	bounds = nvm_addr_check(addr, geo);
	if (bounds) {
		printf("Invalid address: ");
		nvm_addr_pr(addr);
		printf("Exceeds:\n"); nvm_bounds_pr(bounds);
	} else {
		ret = cmd->func(dev, geo, addr, cmd->flags);
	}

	nvm_dev_close(dev);				// close `dev`

	return ret != 0;
}

