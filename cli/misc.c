/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"
#include <nvm_dev.h>


// PPAF	convert LNVM->CNEX
int host2nand(NVM_CLI_CMD_ARGS *args)
{
	uint64_t nandppa = 0;
	uint64_t hostppa = args->addrs[0].ppa;

	printf("Host address:0x%lx\n", hostppa);

	nandppa = nvm_addr_host2nand(args->dev, hostppa);

	printf("Nand address:0x%lx\n", nandppa);	

	return 0;
}

// PPAF convert CNEX->LNVM
int nand2host(NVM_CLI_CMD_ARGS *args)
{
	uint64_t hostppa = 0;
	uint64_t nandppa = args->addrs[0].ppa;

	printf("Nand address:0x%lx\n", nandppa);

	hostppa = nvm_addr_nand2host(args->dev, nandppa);

	printf("Host address:0x%lx\n", hostppa);

	return 0;
}

int read_register(NVM_CLI_CMD_ARGS *args)
{
	uint32_t regaddr = args->offset;
	uint32_t value;

	value = nvme_register_read(args->dev, regaddr);

	printf("regaddr(0x%x): 0x%x\n", regaddr, value);

	return 0;
}

int write_register(NVM_CLI_CMD_ARGS *args)
{
	uint32_t regaddr = args->offset;
	uint32_t value = args->count;

	nvme_register_write(args->dev, regaddr, value);

	return 0;
}

static NVM_CLI_CMD cmds[] = {
	{"host2nand", host2nand, NVM_CLI_ARG_MISC_ADDR, NULL},
	{"nand2host", nand2host, NVM_CLI_ARG_MISC_ADDR, NULL},
	{"writereg", write_register, NVM_CLI_ARG_MISC_WRREG, NULL},	
	{"readreg", read_register, NVM_CLI_ARG_MISC_RDREG, NULL},
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
		nvm_cli_usage(argv[0], "NVM Misc CMDLIST", cmds, ncmds);
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}

