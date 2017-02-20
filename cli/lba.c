/**
 * lba - CLI example for using the logical block adressing functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int cmd_pwrite(NVM_CLI_CMD_ARGS *args)
{
	ssize_t err = 0;
	struct nvm_ret ret;
	char *buf;

	buf = nvm_buf_alloc(args->geo, args->count);
	if (!buf) {
		perror("nvm_buf_alloc");
		return ENOMEM;
	}
	nvm_buf_fill(buf, args->count);

	printf("** nvm_lba_pwrite(...): count(%lu), offset(%ld)\n",
	       args->count, args->offset);

	err = nvm_lba_pwrite(args->dev, buf, args->count, args->offset);
	if (err) {
		perror("nvm_lba_pwrite");
		nvm_ret_pr(&ret);
	}

	free(buf);

	return err ? 1 : 0;
}

int cmd_pread(NVM_CLI_CMD_ARGS *args)
{
	ssize_t err = 0;
	struct nvm_ret ret;
	char *buf;

	buf = nvm_buf_alloc(args->geo, args->count);
	if (!buf) {
		perror("nvm_buf_alloc");
		return ENOMEM;
	}

	printf("** nvm_lba_pread(...): count(%lu), offset(%ld)\n",
	       args->count, args->offset);

	err = nvm_lba_pread(args->dev, buf, args->count, args->offset);
	if (err) {
		perror("nvm_lba_pread");
		nvm_ret_pr(&ret);
	}
	if (getenv("NVM_BUF_PR"))
		nvm_buf_pr(buf, args->count);

	free(buf);

	return err ? 1 : 0;
}

//
// Remaining code is CLI boiler-plate
//

static NVM_CLI_CMD cmds[] = {
	{"pwrite", cmd_pwrite, NVM_CLI_ARG_COUNT_OFFSET, NULL},
	{"pread", cmd_pread, NVM_CLI_ARG_COUNT_OFFSET, NULL},
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);

int main(int argc, char **argv)
{
	NVM_CLI_CMD *cmd;
	int ret = 0;

	cmd = nvm_cli_setup(argc, argv, cmds, ncmds);
	if (cmd) {
		ret = cmd->func(cmd->args);
	} else {
		nvm_cli_usage(argv[0], "NVM logical-block-address (nvm_lba_*)",
			      cmds, ncmds);
		ret = 1;
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}

