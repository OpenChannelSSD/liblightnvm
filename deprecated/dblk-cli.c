#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_cli.h>

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_dblk *dblk;

		printf("** nvm_dblk_erase(...):\n");
		nvm_addr_pr(args->addrs[i]);

		dblk = nvm_dblk_alloc(args->dev, args->addrs[i]);
		if (!dblk) {
			perror("nvm_dblk_alloc");
			return errno;
		}

		if (nvm_dblk_erase(dblk) < 0) {
			perror("nvm_dblk_erase");
			err = errno;
		}

		nvm_dblk_free(dblk);
	}

	return err;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_dblk *dblk;
		char *buf;

		printf("** nvm_dblk_write(...):\n");
		nvm_addr_pr(args->addrs[i]);

		dblk = nvm_dblk_alloc(args->dev, args->addrs[i]);
		if (!dblk) {
			perror("nvm_dblk_alloc");
			return errno;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vblk_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_dblk_free(dblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo->vblk_nbytes);

		if (nvm_dblk_write(dblk, buf, args->geo->vblk_nbytes) < 0) {
			err = errno;
			perror("nvm_dblk_write");
		}

		free(buf);
		nvm_dblk_free(dblk);
	}

	return err;
}

int pwrite(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_dblk *dblk;
		char *buf;
		const size_t offset = args->geo->vpg_nbytes * args->addrs[i].g.pg;

		printf("** nvm_dblk_pwrite(...):\n");
		nvm_addr_pr(args->addrs[i]);

		dblk = nvm_dblk_alloc(args->dev, args->addrs[i]);
		if (!dblk) {
			perror("nvm_dblk_alloc");
			return ENOMEM;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vpg_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_dblk_free(dblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo->vpg_nbytes);

		if (nvm_dblk_pwrite(dblk, buf, args->geo->vpg_nbytes, offset) < 0) {
			perror("nvm_dblk_pwrite");
			err = errno;
		}

		free(buf);
		nvm_dblk_free(dblk);
	}

	return err;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_dblk *dblk;
		void *buf;

		printf("** nvm_dblk_read(...):\n");
		nvm_addr_pr(args->addrs[i]);

		dblk = nvm_dblk_alloc(args->dev, args->addrs[i]);
		if (!dblk) {
			perror("nvm_dblk_alloc");
			return ENOMEM;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vblk_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_dblk_free(dblk);
			return ENOMEM;
		}

		if (nvm_dblk_read(dblk, buf, args->geo->vblk_nbytes) < 0) {
			perror("nvm_dblk_read");
			err = errno;
		}

		if (getenv("NVM_BUF_PR"))
			nvm_buf_pr(buf, args->geo->vblk_nbytes);

		free(buf);
		nvm_dblk_free(dblk);
	}

	return err;
}

int pread(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_dblk *dblk;
		void *buf;
		const size_t offset = args->geo->vpg_nbytes * args->addrs[i].g.pg;

		printf("** nvm_dblk_pread(...):\n");
		nvm_addr_pr(args->addrs[i]);

		dblk = nvm_dblk_alloc(args->dev, args->addrs[i]);
		if (!dblk) {
			perror("nvm_dblk_alloc");
			return ENOMEM;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vpg_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_dblk_free(dblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo->vpg_nbytes);

		if (nvm_dblk_pread(dblk, buf, args->geo->vpg_nbytes, offset) < 0) {
			perror("nvm_dblk_pread");
			err = errno;
		}

		if (getenv("NVM_BUF_PR"))
			nvm_buf_pr(buf, args->geo->vpg_nbytes);

		free(buf);
		nvm_dblk_free(dblk);
	}

	return err;
}

/// CLI boiler-plate

static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_PPALIST, 0x0},
	{"write", write, NVM_CLI_ARG_PPALIST, 0x0},
	{"read", read, NVM_CLI_ARG_PPALIST, 0x0},
	{"pwrite", pwrite, NVM_CLI_ARG_PPALIST, 0x0},
	{"pread", pread, NVM_CLI_ARG_PPALIST, 0x0},
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
		nvm_cli_usage(argv[0], "NVM deprecated block (nvm_dblk_*)",
			      cmds, ncmds);
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}
