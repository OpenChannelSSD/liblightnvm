#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <liblightnvm.h>
#include <liblightnvm_cli.h>

int cmd_attr(struct nvm_cli *cli)
{
	nvm_cli_info_pr("Device information -- nvm_dev_attr_pr");
	printf("dev_"); nvm_dev_attr_pr(cli->args.dev);

	return 0;
}

int cmd_geo(struct nvm_cli *cli)
{
	nvm_cli_info_pr("Device information -- nvm_geo_pr");
	printf("dev_"); nvm_geo_pr(nvm_dev_get_geo(cli->args.dev));

	return 0;
}

int cmd_ppaf(struct nvm_cli *cli)
{
	nvm_cli_info_pr("Device information -- nvm_spec_(ppaf|ppaf_mask)_pr");
	printf("dev_"); nvm_spec_ppaf_nand_pr(nvm_dev_get_ppaf(cli->args.dev));
	printf("dev_"); nvm_spec_ppaf_nand_mask_pr(nvm_dev_get_ppaf_mask(cli->args.dev));

	return 0;
}

int cmd_info(struct nvm_cli *cli)
{
	nvm_cli_info_pr("Device information -- nvm_dev_pr");

	if (cli->opts.brief) {
		printf("dev_"); nvm_dev_attr_pr(cli->args.dev);
		printf("dev_"); nvm_geo_pr(nvm_dev_get_geo(cli->args.dev));
	} else {
		nvm_dev_pr(cli->args.dev);
	}

	return 0;
}

int cmd_list(struct nvm_cli *cli)
{
	nvm_cli_info_pr("Device listing");

	return 0;
}

/**
 * Command-line interface (CLI) boiler-plate
 */

/* Define commands */
static struct nvm_cli_cmd cmds[] = {
	{"list", cmd_list, NVM_CLI_ARG_NONE, NVM_CLI_OPT_HELP},
	{"attr", cmd_attr, NVM_CLI_ARG_DEV_PATH, NVM_CLI_OPT_HELP},
	{"geo", cmd_geo, NVM_CLI_ARG_DEV_PATH, NVM_CLI_OPT_HELP},
	{"ppaf", cmd_ppaf, NVM_CLI_ARG_DEV_PATH, NVM_CLI_OPT_HELP},
	{"info", cmd_info, NVM_CLI_ARG_DEV_PATH, NVM_CLI_OPT_HELP | NVM_CLI_OPT_BRIEF},
};

/* Define the CLI */
static struct nvm_cli cli = {
	.title = "NVM Device (nvm_dev_*)",
	.descr_short = "Retrieve device information",
	.cmds = cmds,
	.ncmds = sizeof(cmds) / sizeof(cmds[0]),
};

/* Initialize and run */
int main(int argc, char **argv)
{
	int res = 0;

	if (nvm_cli_init(&cli, argc, argv) < 0) {
		perror("# FAILED");
		return 1;
	}

	res = nvm_cli_run(&cli);

	nvm_cli_destroy(&cli);

	return res;
}
