#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <liblightnvm.h>
#include <liblightnvm_cli.h>

static int cmd_enum(struct nvm_cli *cli)
{
	nvm_cli_info_pr("Device enumeration");

	printf("Not implemented %p\n", (void *)cli);

	return 0;
}

static int cmd_info(struct nvm_cli *cli)
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


/**
 * Command-line interface (CLI) boiler-plate
 */

/* Define commands */
static struct nvm_cli_cmd cmds[] = {
	{"enum", cmd_enum, NVM_CLI_ARG_NONE, NVM_CLI_OPT_HELP},
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
