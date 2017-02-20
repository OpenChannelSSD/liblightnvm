/**
 * bbt - CLI example for bad-block-table
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int get(NVM_CLI_CMD_ARGS *args)
{
	const struct nvm_bbt* bbt;
	struct nvm_ret ret = {0};

	printf("** nvm_bbt_get(...):\n");

	bbt = nvm_bbt_get(args->dev, args->addrs[0], &ret);
	if (!bbt) {
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		return 1;
	}

	nvm_bbt_pr(bbt);

	return 0;
}

int _set(NVM_CLI_CMD_ARGS *args, enum nvm_bbt_state state)
{
	struct nvm_bbt* bbt;
	struct nvm_ret ret = {0};
	int nupdates;

	printf("** nvm_bbt_set(...):\n");

	// Get bbt state
	bbt = nvm_bbt_alloc_cp(nvm_bbt_get(args->dev, args->addrs[0], &ret));
	if (!bbt) {
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		return 1;
	}

	printf("Current state:\n"); nvm_bbt_pr(bbt);

	for (uint64_t i = 0; i < bbt->nblks; ++i) {
		bbt->blks[i] = state;
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

int set_f(NVM_CLI_CMD_ARGS *args)
{
	return _set(args, NVM_BBT_FREE);
}

int set_b(NVM_CLI_CMD_ARGS *args)
{
	return _set(args, NVM_BBT_BAD);
}

int set_g(NVM_CLI_CMD_ARGS *args)
{
	return _set(args, NVM_BBT_GBAD);
}

int set_d(NVM_CLI_CMD_ARGS *args)
{
	return _set(args, NVM_BBT_DMRK);
}

int set_h(NVM_CLI_CMD_ARGS *args)
{
	return _set(args, NVM_BBT_HMRK);
}

int _mark(NVM_CLI_CMD_ARGS *args, enum nvm_bbt_state state)
{
	ssize_t err = 0;
	struct nvm_ret ret = {0};

	printf("** nvm_bbt_mark(...):\n");
	for (int i = 0; i < args->naddrs; ++i)
		nvm_addr_pr(args->addrs[i]);

	err = nvm_bbt_mark(args->dev, args->addrs, args->naddrs, state, &ret);
	if (err) {
		perror("nvm_bbt_mark");
		nvm_ret_pr(&ret);
	}

	return err ? 1 : 0;
}

int mark_f(NVM_CLI_CMD_ARGS *args)
{
	return _mark(args, NVM_BBT_FREE);
}

int mark_b(NVM_CLI_CMD_ARGS *args)
{
	return _mark(args, NVM_BBT_BAD);
}

int mark_g(NVM_CLI_CMD_ARGS *args)
{
	return _mark(args, NVM_BBT_GBAD);
}

int mark_h(NVM_CLI_CMD_ARGS *args)
{
	return _mark(args, NVM_BBT_HMRK);
}

int mark_d(NVM_CLI_CMD_ARGS *args)
{
	return _mark(args, NVM_BBT_DMRK);
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"get",		get,	NVM_CLI_ARG_CH_LUN,	NULL},
	{"set_f",	set_f,	NVM_CLI_ARG_CH_LUN,	NULL},
	{"set_b",	set_b,	NVM_CLI_ARG_CH_LUN,	NULL},
	{"set_g",	set_g,	NVM_CLI_ARG_CH_LUN,	NULL},
	{"set_d",	set_d,	NVM_CLI_ARG_CH_LUN,	NULL},
	{"set_h",	set_h,	NVM_CLI_ARG_CH_LUN,	NULL},
	{"mark_f",	mark_f,	NVM_CLI_ARG_ADDRLIST,	NULL},
	{"mark_b",	mark_b,	NVM_CLI_ARG_ADDRLIST,	NULL},
	{"mark_g",	mark_g,	NVM_CLI_ARG_ADDRLIST,	NULL},
	{"mark_d",	mark_d,	NVM_CLI_ARG_ADDRLIST,	NULL},
	{"mark_h",	mark_h,	NVM_CLI_ARG_ADDRLIST,	NULL},
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
		nvm_cli_usage(argv[0], "NVM bad-block-table (nvm_bbt_*)", cmds,
			      ncmds);
		ret = 1;
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}

