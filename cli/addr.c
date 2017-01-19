/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	struct nvm_ret ret;
	int PMODE;
	ssize_t err = 0;

	PMODE = nvm_cli_pmode(args->dev);
	if (PMODE < 0) {
		perror("nvm_cli_pmode");
		return EINVAL;
	}

	printf("** nvm_addr_erase(...) : pmode(0x%x)\n", PMODE);
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_addr_erase(args->dev, args->addrs, args->naddrs, PMODE,
			     &ret);
	if (err) {
		perror("nvm_addr_erase");
		nvm_ret_pr(&ret);
	}

	return err ? 1 : 0;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	struct nvm_ret ret;
	int PMODE;
	ssize_t err = 0;

	PMODE = nvm_cli_pmode(args->dev);
	if (PMODE < 0) {
		perror("nvm_cli_pmode");
		return EINVAL;
	}

	int buf_nbytes = args->naddrs * args->geo->sector_nbytes;
	char *buf = NULL;
	int meta_tbytes = args->naddrs * args->geo->meta_nbytes;
	char *meta = NULL;

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

	printf("** nvm_addr_write(...) : pmode(0x%x)\n", PMODE);
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	if (getenv("NVM_CLI_BUF_PR")) {
		printf("** Writing buffer:\n");
		nvm_buf_pr(buf, buf_nbytes);
	}
	if (meta && getenv("NVM_CLI_META_PR")) {
		printf("** Writing meta:\n");
		nvm_buf_pr(meta, meta_tbytes);
	}

	err = nvm_addr_write(args->dev, args->addrs, args->naddrs, buf, meta,
			     PMODE, &ret);
	if (err) {
		perror("nvm_addr_write");
		nvm_ret_pr(&ret);
	}

	free(buf);
	free(meta);

	return err ? 1 : 0;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	struct nvm_ret ret;
	int PMODE;
	ssize_t err = 0;

	PMODE = nvm_cli_pmode(args->dev);
	if (PMODE < 0) {
		perror("nvm_cli_pmode");
		return EINVAL;
	}

	int buf_nbytes = args->naddrs * args->geo->sector_nbytes;
	char *buf = NULL;
	int meta_tbytes = args->naddrs * args->geo->meta_nbytes;
	char *meta = NULL;

	buf = nvm_buf_alloc(args->geo, buf_nbytes);	// data buffer
	if (!buf)
		return ENOMEM;

	if (flags & 0x1) {				// metadata buffer
		meta = nvm_buf_alloc(args->geo, meta_tbytes);
		if (!meta) {
			free(buf);
			return ENOMEM;
		}
		memset(meta, 0, meta_tbytes);
	}

	printf("** nvm_addr_read(...) : pmode(0x%x)\n", PMODE);
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_addr_read(args->dev, args->addrs, args->naddrs, buf, meta,
			    PMODE, NULL);
	if (err) {
		perror("nvm_addr_read");
		nvm_ret_pr(&ret);
	}
	
	if (getenv("NVM_CLI_BUF_PR")) {
		printf("** Read buffer:\n");
		nvm_buf_pr(buf, buf_nbytes);
	}
	if (meta && getenv("NVM_CLI_META_PR")) {
		printf("** Read meta_tbytes(%d) meta:\n", meta_tbytes);
		nvm_buf_pr(meta, meta_tbytes);
	}

	free(buf);
	free(meta);

	return err;
}

int cmd_fmt(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->naddrs; ++i)
		nvm_addr_pr(args->addrs[i]);

	return 0;
}

int cmd_gen2dev(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen-addr"); nvm_addr_pr(args->addrs[i]);
		printf("dev-addr(0x%016lx)\n",
		       nvm_addr_gen2dev(args->dev, args->addrs[i]));
	}

	return 0;
}

int cmd_gen2lba(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen-addr"); nvm_addr_pr(args->addrs[i]);
		printf("lba-addr(%064ld)\n",
		       nvm_addr_gen2lba(args->dev, args->addrs[i]));
	}

	return 0;
}

int cmd_gen2off(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen-addr"); nvm_addr_pr(args->addrs[i]);
		printf("off-addr(%064ld)\n",
		       nvm_addr_gen2off(args->dev, args->addrs[i]));
	}

	return 0;
}

int cmd_dev2gen(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->nlbas; ++i) {
		printf("dev-addr(%064ld)\n", args->lbas[i]);
		nvm_addr_pr(nvm_addr_dev2gen(args->dev, args->lbas[i]));
	}

	return 0;
}

int cmd_lba2gen(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->nlbas; ++i) {
		printf("lba-addr(%064ld)\n", args->lbas[i]);
		nvm_addr_pr(nvm_addr_lba2gen(args->dev, args->lbas[i]));
	}

	return 0;
}

int cmd_off2gen(NVM_CLI_CMD_ARGS *args, int flags)
{
	for (int i = 0; i < args->nlbas; ++i) {
		printf("off-addr(%064ld)\n", args->lbas[i]);
		nvm_addr_pr(nvm_addr_off2gen(args->dev, args->lbas[i]));
	}

	return 0;
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_ADDRLIST, 0x0},
	{"write", write, NVM_CLI_ARG_ADDRLIST, 0x1},
	{"read", read, NVM_CLI_ARG_ADDRLIST, 0x1},
	{"write_wm", write, NVM_CLI_ARG_ADDRLIST, 0x0},
	{"read_wm", read, NVM_CLI_ARG_ADDRLIST, 0x0},
	{"from_hex", cmd_fmt, NVM_CLI_ARG_ADDRLIST, 0x0},
	{"from_geo", cmd_fmt, NVM_CLI_ARG_CH_LUN_PL_BLK_PG_SEC, 0x0},
	{"gen2dev", cmd_gen2dev, NVM_CLI_ARG_ADDRLIST, 0x0},
	{"gen2lba", cmd_gen2lba, NVM_CLI_ARG_ADDRLIST, 0x0},
	{"gen2off", cmd_gen2off, NVM_CLI_ARG_ADDRLIST, 0x0},
	{"dev2gen", cmd_dev2gen, NVM_CLI_ARG_INTLIST, 0x0},
	{"lba2gen", cmd_lba2gen, NVM_CLI_ARG_INTLIST, 0x0},
	{"off2gen", cmd_off2gen, NVM_CLI_ARG_INTLIST, 0x0},
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
		ret = 1;
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}

