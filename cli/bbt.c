/**
 * bbt - CLI example for bad-block-table
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int get(NVM_CLI_CMD_ARGS *args, int flags)
{
	struct nvm_bbt* bbt;
	struct nvm_ret ret;

	printf("** nvm_bbt_get(...):\n");

	bbt = nvm_bbt_get(args->dev, args->addrs[0], &ret);
	if (!bbt) {
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		return 1;
	}

	nvm_bbt_pr(bbt);
	nvm_bbt_free(bbt);

	return 0;
}

int set(NVM_CLI_CMD_ARGS *args, int flags)
{
	struct nvm_bbt* bbt;
	struct nvm_ret ret;
	int nupdates;

	printf("** nvm_bbt_set(...):\n");

	bbt = nvm_bbt_get(args->dev, args->addrs[0], &ret);	// Get bbt state
	if (!bbt) {
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		return 1;
	}

	printf("Current state:\n"); nvm_bbt_pr(bbt);

	for (int i = 0; i < bbt->nblks; ++i) {
		bbt->blks[i] = flags;
	}

	printf("New state:\n"); nvm_bbt_pr(bbt);

	nupdates = nvm_bbt_set(args->dev, bbt, &ret);	// Persist bbt on device
	if (nupdates < 0) {
		perror("nvm_bbt_set");
		nvm_ret_pr(&ret);
	} else {
		printf("** SUCCESS -- nupdates(%d)\n", nupdates);
	}

	nvm_bbt_free(bbt);

	return 0;
}

int mark(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;
	struct nvm_ret ret;

	printf("** nvm_bbt_mark(...):\n");
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_bbt_mark(args->dev, args->addrs, args->naddrs, flags, &ret);
	if (err) {
		perror("nvm_addr_erase");
		nvm_ret_pr(&ret);
	}

	return err ? 1 : 0;
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"get", get, NVM_CLI_ARG_CH_LUN, 0x0},
	{"set_f", set, NVM_CLI_ARG_CH_LUN, NVM_BBT_FREE},
	{"set_b", set, NVM_CLI_ARG_CH_LUN, NVM_BBT_BAD},
	{"set_g", set, NVM_CLI_ARG_CH_LUN, NVM_BBT_GBAD},
	{"set_d", set, NVM_CLI_ARG_CH_LUN, NVM_BBT_DBAD},
	{"set_h", set, NVM_CLI_ARG_CH_LUN, NVM_BBT_HBAD},
	{"mark_f", mark, NVM_CLI_ARG_PPALIST, NVM_BBT_FREE},
	{"mark_b", mark, NVM_CLI_ARG_PPALIST, NVM_BBT_BAD},
	{"mark_g", mark, NVM_CLI_ARG_PPALIST, NVM_BBT_GBAD},
	{"mark_d", mark, NVM_CLI_ARG_PPALIST, NVM_BBT_DBAD},
	{"mark_h", mark, NVM_CLI_ARG_PPALIST, NVM_BBT_HBAD},
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
		nvm_cli_usage(argv[0], "NVM bad-block-table (nvm_bbt_*)", cmds,
			      ncmds);
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}

