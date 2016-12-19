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
		struct nvm_vblk *vblk;

		printf("** nvm_vblk_erase(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return errno;
		}

		if (nvm_vblk_erase(vblk) < 0) {
			perror("nvm_vblk_erase");
			err = errno;
		}

		nvm_vblk_free(vblk);
	}

	return err;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_vblk *vblk;
		char *buf;

		printf("** nvm_vblk_write(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return errno;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vblk_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_vblk_free(vblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo->vblk_nbytes);

		if (nvm_vblk_write(vblk, buf, args->geo->vblk_nbytes) < 0) {
			err = errno;
			perror("nvm_vblk_write");
		}

		free(buf);
		nvm_vblk_free(vblk);
	}

	return err;
}

int pwrite(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_vblk *vblk;
		char *buf;
		const size_t offset = args->geo->vpg_nbytes * args->addrs[i].g.pg;

		printf("** nvm_vblk_pwrite(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return ENOMEM;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vpg_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_vblk_free(vblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo->vpg_nbytes);

		if (nvm_vblk_pwrite(vblk, buf, args->geo->vpg_nbytes, offset) < 0) {
			perror("nvm_vblk_pwrite");
			err = errno;
		}

		free(buf);
		nvm_vblk_free(vblk);
	}

	return err;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_vblk *vblk;
		void *buf;

		printf("** nvm_vblk_read(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return ENOMEM;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vblk_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_vblk_free(vblk);
			return ENOMEM;
		}

		if (nvm_vblk_read(vblk, buf, args->geo->vblk_nbytes) < 0) {
			perror("nvm_vblk_read");
			err = errno;
		}

		if (getenv("NVM_BUF_PR"))
			nvm_buf_pr(buf, args->geo->vblk_nbytes);

		free(buf);
		nvm_vblk_free(vblk);
	}

	return err;
}

int pread(NVM_CLI_CMD_ARGS *args, int flags)
{
	int err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_vblk *vblk;
		void *buf;
		const size_t offset = args->geo->vpg_nbytes * args->addrs[i].g.pg;

		printf("** nvm_vblk_pread(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return ENOMEM;
		}

		buf = nvm_buf_alloc(args->geo, args->geo->vpg_nbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			nvm_vblk_free(vblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo->vpg_nbytes);

		if (nvm_vblk_pread(vblk, buf, args->geo->vpg_nbytes, offset) < 0) {
			perror("nvm_vblk_pread");
			err = errno;
		}

		if (getenv("NVM_BUF_PR"))
			nvm_buf_pr(buf, args->geo->vpg_nbytes);

		free(buf);
		nvm_vblk_free(vblk);
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
		nvm_cli_usage(argv[0], "NVM virtual block (nvm_vblk_*)", cmds,
			      ncmds);
	}
	
	nvm_cli_teardown(cmd);

	return ret != 0;
}
