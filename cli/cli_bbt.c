/**
 * bbt - CLI example for bad-block-table
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "liblightnvm_cli.h"

int get(struct nvm_cli *cli)
{
	const struct nvm_bbt* bbt;
	struct nvm_ret ret = {0,0};

	printf("** nvm_bbt_get(...):\n");

	bbt = nvm_bbt_get(cli->args.dev, cli->args.addrs[0], &ret);
	if (!bbt) {
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		return -1;
	}

	nvm_bbt_pr(bbt);

	return 0;
}

int _set(struct nvm_cli_cmd_args *args, enum nvm_bbt_state state)
{
	struct nvm_bbt* bbt;
	struct nvm_ret ret = {0,0};
	int nupdates;

	printf("** nvm_bbt_set(...):\n");

	// Get bbt state
	bbt = nvm_bbt_alloc_cp(nvm_bbt_get(args->dev, args->addrs[0], &ret));
	if (!bbt) {
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		return -1;
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

int set_f(struct nvm_cli *cli)
{
	return _set(&cli->args, NVM_BBT_FREE);
}

int set_b(struct nvm_cli *cli)
{
	return _set(&cli->args, NVM_BBT_BAD);
}

int set_g(struct nvm_cli *cli)
{
	return _set(&cli->args, NVM_BBT_GBAD);
}

int set_d(struct nvm_cli *cli)
{
	return _set(&cli->args, NVM_BBT_DMRK);
}

int set_h(struct nvm_cli *cli)
{
	return _set(&cli->args, NVM_BBT_HMRK);
}

int _mark(struct nvm_cli_cmd_args *args, enum nvm_bbt_state state)
{
	ssize_t err = 0;
	struct nvm_ret ret = {0,0};

	printf("** nvm_bbt_mark(...):\n");
	for (int i = 0; i < args->naddrs; ++i)
		nvm_addr_pr(args->addrs[i]);

	err = nvm_bbt_mark(args->dev, args->addrs, args->naddrs, state, &ret);
	if (err) {
		perror("nvm_bbt_mark");
		nvm_ret_pr(&ret);
	}

	return err;
}

int mark_f(struct nvm_cli *cli)
{
	return _mark(&cli->args, NVM_BBT_FREE);
}

int mark_b(struct nvm_cli *cli)
{
	return _mark(&cli->args, NVM_BBT_BAD);
}

int mark_g(struct nvm_cli *cli)
{
	return _mark(&cli->args, NVM_BBT_GBAD);
}

int mark_h(struct nvm_cli *cli)
{
	return _mark(&cli->args, NVM_BBT_HMRK);
}

int mark_d(struct nvm_cli *cli)
{
	return _mark(&cli->args, NVM_BBT_DMRK);
}

//
// Remaining code is CLI boiler-plate
//
static struct nvm_cli_cmd cmds[] = {
	{"get",		get,	NVM_CLI_ARG_ADDR_LUN,	NVM_CLI_OPT_DEFAULT},
	{"set_f",	set_f,	NVM_CLI_ARG_ADDR_LUN,	NVM_CLI_OPT_DEFAULT},
	{"set_b",	set_b,	NVM_CLI_ARG_ADDR_LUN,	NVM_CLI_OPT_DEFAULT},
	{"set_g",	set_g,	NVM_CLI_ARG_ADDR_LUN,	NVM_CLI_OPT_DEFAULT},
	{"set_d",	set_d,	NVM_CLI_ARG_ADDR_LUN,	NVM_CLI_OPT_DEFAULT},
	{"set_h",	set_h,	NVM_CLI_ARG_ADDR_LUN,	NVM_CLI_OPT_DEFAULT},
	{"mark_f",	mark_f,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT},
	{"mark_b",	mark_b,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT},
	{"mark_g",	mark_g,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT},
	{"mark_d",	mark_d,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT},
	{"mark_h",	mark_h,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT},
};

static struct nvm_cli cli = {
	.title = "NVM bad-block-table (nvm_bbt_*)",
	.descr_short = "Retrieve (get) and modify (set/mark) the bad block table",
	.cmds = cmds,
	.ncmds = sizeof(cmds) / sizeof(cmds[0]),
};

/* Initialize and run */
int main(int argc, char **argv)
{
	int res = 0;

	if (nvm_cli_init(&cli, argc, argv) < 0) {
		perror("FAILED");
		return 1;
	}

	res = nvm_cli_run(&cli);
	if (res)
		perror(cli.cmd.name);
	
	nvm_cli_destroy(&cli);

	return res;
}
