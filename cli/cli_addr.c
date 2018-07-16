#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm_cli.h>

static int cmd_fmt(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	nvm_addr_prn(args->addrs, args->naddrs, args->dev);

	return 0;
}

static int cmd_gen2dev(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen: ");
		nvm_addr_pr(args->addrs[i]);
		printf("dev: 0x%016"PRIx64"\n", nvm_addr_gen2dev(args->dev, args->addrs[i]));
	}

	return 0;
}

static int cmd_gen2lba(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen: "); nvm_addr_pr(args->addrs[i]);
		printf("lba: %064"PRIu64"\n", nvm_addr_gen2lba(args->dev, args->addrs[i]));
	}

	return 0;
}

static int cmd_gen2off(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen: "); nvm_addr_pr(args->addrs[i]);
		printf("off: %064"PRIu64"\n", nvm_addr_gen2off(args->dev, args->addrs[i]));
	}

	return 0;
}

static int cmd_gen2lpo(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen: "); nvm_addr_pr(args->addrs[i]);
		printf("lpo: %064"PRIu64"\n", nvm_addr_gen2lpo(args->dev, args->addrs[i]));
	}

	return 0;
}

static int cmd_dev2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->nhex_vals; ++i) {
		struct nvm_addr gen = { 0 };

		gen = nvm_addr_dev2gen(args->dev, args->hex_vals[i]);

		printf("dev: 0x%016"PRIx64"\n", args->hex_vals[i]);
		printf("gen: ");
		nvm_addr_prn(&gen, 1, args->dev);
	}

	return 0;
}

static int cmd_lba2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->ndec_vals; ++i) {
		struct nvm_addr gen = { 0 };

		gen = nvm_addr_lba2gen(args->dev, args->dec_vals[i]);

		printf("lba: %064"PRIu64"\n", args->dec_vals[i]);
		printf("gen: ");
		nvm_addr_prn(&gen, 1, args->dev);
	}

	return 0;
}

static int cmd_off2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->ndec_vals; ++i) {
		struct nvm_addr gen = { 0 };

		gen = nvm_addr_off2gen(args->dev, args->dec_vals[i]);
		
		printf("off: %064"PRIu64"\n", args->dec_vals[i]);
		printf("gen: ");
		nvm_addr_prn(&gen, 1, args->dev);
	}

	return 0;
}

static int cmd_lpo2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->ndec_vals; ++i) {
		struct nvm_addr gen = { 0 };

		gen = nvm_addr_lpo2gen(args->dev, args->dec_vals[i]);
		
		printf("off: %064"PRIu64"\n", args->dec_vals[i]);
		printf("gen: ");
		nvm_addr_prn(&gen, 1, args->dev);
	}

	return 0;
}

/**
 * Command-line interface (CLI) boiler-plate
 */

// MOVE: erase, write, write_wm, read, read_wm to nvm_cmd *

/* Define commands */
static struct nvm_cli_cmd cmds[] = {

	{"s12_to_gen",	cmd_fmt,	NVM_CLI_ARG_ADDR_S12, NVM_CLI_OPT_DEFAULT},
	{"s20_to_gen",	cmd_fmt,	NVM_CLI_ARG_ADDR_S20, NVM_CLI_OPT_DEFAULT},

	//{"from_hex",		cmd_fmt,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	
	{"gen2dev",	cmd_gen2dev,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"gen2lba",	cmd_gen2lba,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"gen2off",	cmd_gen2off,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"gen2lpo",	cmd_gen2lpo,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},

	{"dev2gen",	cmd_dev2gen,	NVM_CLI_ARG_HEXVAL_LIST, NVM_CLI_OPT_DEFAULT},
	{"lba2gen",	cmd_lba2gen,	NVM_CLI_ARG_DECVAL_LIST, NVM_CLI_OPT_DEFAULT},
	{"off2gen",	cmd_off2gen,	NVM_CLI_ARG_DECVAL_LIST, NVM_CLI_OPT_DEFAULT},
	{"lpo2gen",	cmd_lpo2gen,	NVM_CLI_ARG_DECVAL_LIST, NVM_CLI_OPT_DEFAULT},
};

/* Define the CLI */
static struct nvm_cli cli = {
	.title = "NVM address (nvm_addr_*)",
	.descr_short = "Construct and convert addresses",
	.cmds = cmds,
	.ncmds = sizeof(cmds) / sizeof(cmds[0]),
};

/* Initialize and run */
int main(int argc, char **argv)
{
	int res = 0;

	if (nvm_cli_init(&cli, argc, argv) < 0) {
		nvm_cli_perror("nvm_cli_init");
		return 1;
	}

	res = nvm_cli_run(&cli);

	nvm_cli_destroy(&cli);

	return res;
}

