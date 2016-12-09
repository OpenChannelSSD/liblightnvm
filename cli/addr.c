/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	NVM_RET ret;
	int PLANE_FLAG;
	ssize_t err = 0;

	switch (args->geo.nplanes) {
	case 4:
		PLANE_FLAG = NVM_MAGIC_FLAG_QUAD;
		break;
	case 2:
		PLANE_FLAG = NVM_MAGIC_FLAG_DUAL;
		break;
	default:
		PLANE_FLAG = NVM_MAGIC_FLAG_SNGL;
	}

	printf("** nvm_addr_erase(...):\n");
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_addr_erase(args->dev, args->addrs, args->naddrs, PLANE_FLAG,
			     &ret);
	if (err) {
		perror("nvm_addr_erase");
		nvm_ret_pr(&ret);
	}

	return err ? 1 : 0;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	NVM_RET ret;
	int PLANE_FLAG;
	ssize_t err = 0;

	int buf_nbytes = args->naddrs * args->geo.nbytes;
	char *buf = NULL;
	int meta_tbytes = args->naddrs * args->geo.meta_nbytes;
	char *meta = NULL;

	switch (args->geo.nplanes) {
	case 4:
		PLANE_FLAG = NVM_MAGIC_FLAG_QUAD;
		break;
	case 2:
		PLANE_FLAG = NVM_MAGIC_FLAG_DUAL;
		break;
	default:
		PLANE_FLAG = NVM_MAGIC_FLAG_SNGL;
	}

	buf = nvm_buf_alloc(args->geo, buf_nbytes);	// data buffer
	if (!buf)
		return ENOMEM;
	nvm_buf_fill(buf, buf_nbytes);

	if (flags & 0x1) {				// metadata buffer
		meta = nvm_buf_alloc(args->geo, meta_tbytes);
		if (!meta) {
			free(buf);
			return ENOMEM;
		}
		for (int i = 0; i < meta_tbytes; ++i)
			meta[i] = (i / args->naddrs) % args->naddrs + 65;
	}

	printf("** nvm_addr_write(...):\n");
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}
	
	err = nvm_addr_write(args->dev, args->addrs, args->naddrs, buf, meta,
			     PLANE_FLAG, &ret);
	if (err) {
		perror("nvm_addr_write");
		nvm_ret_pr(&ret);
	}

	if (meta) {
		printf("meta(%d) {", meta_tbytes);
		for (int i = 0; i < meta_tbytes; ++i) {
			if (i)
				printf(",");
			printf(" %c", meta[i]);
		}
		printf(" }\n");
	}

	free(buf);
	free(meta);

	return err ? 1 : 0;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	NVM_RET ret;
	int PLANE_FLAG;
	ssize_t err = 0;

	int buf_nbytes = args->naddrs * args->geo.nbytes;
	char *buf = NULL;
	int meta_tbytes = args->naddrs * args->geo.meta_nbytes;
	char *meta = NULL;

	switch (args->geo.nplanes) {
	case 4:
		PLANE_FLAG = NVM_MAGIC_FLAG_QUAD;
		break;
	case 2:
		PLANE_FLAG = NVM_MAGIC_FLAG_DUAL;
		break;
	default:
		PLANE_FLAG = NVM_MAGIC_FLAG_SNGL;
	}

	buf = nvm_buf_alloc(args->geo, buf_nbytes);	// data buffer
	if (!buf)
		return ENOMEM;

	if (flags & 0x1) {				// metadata buffer
		meta = nvm_buf_alloc(args->geo, meta_tbytes);
		if (!meta) {
			free(buf);
			return ENOMEM;
		}
	}

	printf("** nvm_addr_read(...): \n");
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_addr_read(args->dev, args->addrs, args->naddrs, buf, meta,
			    PLANE_FLAG, NULL);
	if (err) {
		perror("nvm_addr_read");
		nvm_ret_pr(&ret);
	}
	
	if (getenv("NVM_BUF_PR")) {
		nvm_buf_pr(buf, buf_nbytes);
		if (meta) {
			printf("meta {");
			for (int i = 0; i < meta_tbytes; ++i) {
				if (i)
					printf(",");
				printf(" %c", meta[i]);
			}
			printf(" }\n");
		}
	}

	free(buf);
	free(meta);

	return err;
}

int cmd_fmt(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	return 0;
}

int cmd_lba(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
		printf("lba(%lu)\n",
		       nvm_addr_gen2lba(args->dev, args->addrs[i]));
	}

	return 0;
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_PPALIST, 0x0},
	{"write", write, NVM_CLI_ARG_PPALIST, 0x0},
	{"read", read, NVM_CLI_ARG_PPALIST, 0x0},
	{"write_m", write, NVM_CLI_ARG_PPALIST, 0x1},
	{"read_m", read, NVM_CLI_ARG_PPALIST, 0x1},
	{"hex2gen", cmd_fmt, NVM_CLI_ARG_PPALIST, 0x0},
	{"gen2hex", cmd_fmt, NVM_CLI_ARG_CH_LUN_PL_BLK_PG_SEC, 0x0},
	{"hex2lba", cmd_lba, NVM_CLI_ARG_PPALIST, 0x0},
	{"gen2lba", cmd_lba, NVM_CLI_ARG_CH_LUN_PL_BLK_PG_SEC, 0x0}
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
		nvm_cli_usage(argv[0], "NVM address (nvm_addr_*)", cmds, ncmds);
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}

