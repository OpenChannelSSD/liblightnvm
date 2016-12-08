#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err;

	printf("** nvm_sblk_erase(...): sblk_tbytes(%lu)\n",
	        args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);

	nvm_timer_start();
	err = nvm_sblk_erase(args->sblk);
	if (err)
		printf("FAILED: nvm_sblk_erase err(%ld)\n", err);
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_erase");

	return err ? 1 : 0;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err;
	char *buf;

	printf("** nvm_sblk_write(...): sblk_tbytes(%lu)\n",
	       args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);
	
	nvm_timer_start();
	buf = nvm_buf_alloc(args->sblk_geo, args->sblk_geo.tbytes);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		return ENOMEM;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	nvm_buf_fill(buf, args->sblk_geo.tbytes);
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_fill");

	nvm_timer_start();
	err = nvm_sblk_write(args->sblk, buf, args->sblk_geo.tbytes);
	if (err)
		printf("FAILED: nvm_sblk_write err(%ld)\n", err);
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_write");

	free(buf);

	return err ? 1 : 0;
}

int pad(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err;

	printf("** nvm_sblk_pad(...): sblk_tbytes(%lu)\n",
	       args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);

	nvm_timer_start();
	err = nvm_sblk_pad(args->sblk);
	if (err) {
		printf("FAILED: nvm_sblk_pad err(%ld)\n", err);
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_pad");

	return err ? 1 : 0;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err;
	char *buf;

	printf("** nvm_sblk_read(...): sblk_tbytes(%lu)\n",
	       args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);

	nvm_timer_start();
	buf = nvm_buf_alloc(args->sblk_geo, args->sblk_geo.tbytes);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		return ENOMEM;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	err = nvm_sblk_read(args->sblk, buf, args->sblk_geo.tbytes);
	if (err) {
		printf("FAILED: nvm_sblk_read err(%ld)\n", err);
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_read");

	free(buf);

	return err ? 1 : 0;
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_SBLK, 0x0},
	{"write", write, NVM_CLI_ARG_SBLK, 0x0},
	{"pad", pad, NVM_CLI_ARG_SBLK, 0x0},
	{"read", read, NVM_CLI_ARG_SBLK, 0x0},
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);

int main(int argc, char **argv)
{
	NVM_CLI_CMD *cmd;
	int ret = 0;

	cmd = nvm_cli_setup(argc, argv, cmds, ncmds);
	if (cmd) {
		ret = cmd->func(&cmd->args, cmd->flags);
	} else {
		nvm_cli_usage(argv[0], "NVM spanning block (nvm_sblk_*)", cmds,
			      ncmds);
	}
	nvm_cli_teardown(cmd);

	return ret != 0;
}

