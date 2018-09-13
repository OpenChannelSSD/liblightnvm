#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm_cli.h>

static int cmd_fmt(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	if (cli->opts.terse) {
		nvm_addr_pr((args->addrs[0]));
		printf("\n");
		return 0;
	}

	nvm_addr_prn(args->addrs, args->naddrs, args->dev);

	return 0;
}

static int cmd_dfmt(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;
	
	nvm_cli_info_pr("NOTICE: addresses are converted to gen to print geo");

	for (int i = 0; i < args->nhex_vals; ++i) {
		args->addrs[i] = nvm_addr_dev2gen(args->dev, args->hex_vals[i]);
	}
	args->naddrs = args->nhex_vals;

	nvm_addr_prn(args->addrs, args->naddrs, args->dev);

	return 0;
}

static int cmd_gen2dev(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	if (cli->opts.terse) {
		for (int i = 0; i < args->naddrs; ++i) {
			printf("0x%016"PRIx64"\n",
			       nvm_addr_gen2dev(args->dev, args->addrs[i]));
		}
		return 0;
	}

	for (int i = 0; i < args->naddrs; ++i) {
		printf("- {gen: ");
		nvm_addr_pr(args->addrs[i]);
		printf(", dev: 0x%016"PRIx64"}\n",
		       nvm_addr_gen2dev(args->dev, args->addrs[i]));
	}

	return 0;
}

static int cmd_gen2off(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	if (cli->opts.terse) {
		for (int i = 0; i < args->naddrs; ++i) {
			printf("%064"PRIu64"\n",
			       nvm_addr_gen2off(args->dev, args->addrs[i]));
		}
		return 0;
	}

	for (int i = 0; i < args->naddrs; ++i) {
		printf("- {gen: ");
		nvm_addr_pr(args->addrs[i]);
		printf(", off: %064"PRIu64"}\n",
		       nvm_addr_gen2off(args->dev, args->addrs[i]));
	}

	return 0;
}

static int cmd_gen2lpo(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	if (cli->opts.terse) {
		for (int i = 0; i < args->naddrs; ++i) {
			printf("%064"PRIu64"\n",
			       nvm_addr_gen2lpo(args->dev, args->addrs[i]));
		}
		return 0;
	}

	for (int i = 0; i < args->naddrs; ++i) {
		printf("- {gen: ");
		nvm_addr_pr(args->addrs[i]);
		printf(", lpo: %064"PRIu64"}\n",
		       nvm_addr_gen2lpo(args->dev, args->addrs[i]));
	}

	return 0;
}

static int cmd_dev2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	if (cli->opts.terse) {
		for (int i = 0; i < args->nhex_vals; ++i) {
			struct nvm_addr gen = { 0 };
			gen = nvm_addr_dev2gen(args->dev, args->hex_vals[i]);
			nvm_addr_pr(gen);
			printf("\n");
		}
		return 0;
	}

	for (int i = 0; i < args->nhex_vals; ++i) {
		struct nvm_addr gen = { 0 };

		gen = nvm_addr_dev2gen(args->dev, args->hex_vals[i]);

		printf("- {dev: 0x%016"PRIx64, args->hex_vals[i]);
		printf(", gen: ");
		nvm_addr_pr(gen);
		printf("}\n");
	}

	return 0;
}

static int cmd_off2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	if (cli->opts.terse) {
		for (int i = 0; i < args->ndec_vals; ++i) {
			struct nvm_addr gen = { 0 };
			gen = nvm_addr_off2gen(args->dev, args->dec_vals[i]);
			nvm_addr_pr(gen);
			printf("\n");
		}
		return 0;
	}

	for (int i = 0; i < args->ndec_vals; ++i) {
		struct nvm_addr gen = { 0 };

		gen = nvm_addr_off2gen(args->dev, args->dec_vals[i]);

		printf("- {off: %064"PRIu64, args->dec_vals[i]);
		printf(", gen: ");
		nvm_addr_pr(gen);
		printf("}\n");
	}

	return 0;
}

static int cmd_lpo2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	if (cli->opts.terse) {
		for (int i = 0; i < args->ndec_vals; ++i) {
			struct nvm_addr gen = { 0 };
			gen = nvm_addr_lpo2gen(args->dev, args->dec_vals[i]);
			nvm_addr_pr(gen);
			printf("\n");
		}
		return 0;
	}

	for (int i = 0; i < args->ndec_vals; ++i) {
		struct nvm_addr gen = { 0 };

		gen = nvm_addr_lpo2gen(args->dev, args->dec_vals[i]);

		printf("- {off: %064"PRIu64, args->dec_vals[i]);
		printf(", gen: ");
		nvm_addr_pr(gen);
		printf("}\n");
	}

	return 0;
}

/**
 * Command-line interface (CLI) boiler-plate
 */

// MOVE: erase, write, write_wm, read, read_wm to nvm_cmd *

/* Define commands */
static struct nvm_cli_cmd cmds[] = {

	{"s12_to_gen",	cmd_fmt,	NVM_CLI_ARG_ADDR_S12, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},
	{"s20_to_gen",	cmd_fmt,	NVM_CLI_ARG_ADDR_S20, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},

	{"gen2dev",	cmd_gen2dev,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},
	{"dev2gen",	cmd_dev2gen,	NVM_CLI_ARG_HEXVAL_LIST, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},

	{"gen2off",	cmd_gen2off,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},
	{"off2gen",	cmd_off2gen,	NVM_CLI_ARG_DECVAL_LIST, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},

	{"gen2lpo",	cmd_gen2lpo,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},
	{"lpo2gen",	cmd_lpo2gen,	NVM_CLI_ARG_DECVAL_LIST, NVM_CLI_OPT_DEFAULT|NVM_CLI_OPT_TERSE},

	{"gen_as_geo",	cmd_fmt,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"dev_as_geo",	cmd_dfmt,	NVM_CLI_ARG_HEXVAL_LIST, NVM_CLI_OPT_DEFAULT},
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
