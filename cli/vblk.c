#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include <nvm_cli.h>

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		NVM_VBLK vblk;

		printf("** nvm_vblk_erase(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_new_on_dev(args->dev, args->addrs[i]);
		if (!vblk)
			return ENOMEM;

		err = nvm_vblk_erase(vblk);
		if (err)
			printf("FAILED: nvm_vblk_erase err(%ld)\n", err);

		nvm_vblk_free(vblk);
	}

	return err ? 1 : 0;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		NVM_VBLK vblk;
		char *buf;

		printf("** nvm_vblk_write(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_new_on_dev(args->dev, args->addrs[i]);
		if (!vblk)
			return ENOMEM;

		buf = nvm_buf_alloc(args->geo, args->geo.vblk_nbytes);
		if (!buf) {
			nvm_vblk_free(vblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo.vblk_nbytes);

		err = nvm_vblk_write(vblk, buf, args->geo.vblk_nbytes);
		if (err)
			printf("FAILED: nvm_vblk_write err(%ld)\n", err);

		free(buf);
		nvm_vblk_free(vblk);
	}

	return err ? 1 : 0;
}

int pwrite(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		NVM_VBLK vblk;
		char *buf;
		const size_t offset = args->geo.vpg_nbytes * args->addrs[i].g.pg;

		printf("** nvm_vblk_pwrite(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_new_on_dev(args->dev, args->addrs[i]);
		if (!vblk)
			return ENOMEM;

		buf = nvm_buf_alloc(args->geo, args->geo.vpg_nbytes);
		if (!buf) {
			nvm_vblk_free(vblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo.vpg_nbytes);

		err = nvm_vblk_pwrite(vblk, buf, args->geo.vpg_nbytes, offset);
		if (err)
			printf("FAILED: nvm_vblk_pwrite err(%ld)\n", err);

		free(buf);
		nvm_vblk_free(vblk);
	}

	return err ? 1 : 0;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		NVM_VBLK vblk;
		void *buf;

		printf("** nvm_vblk_read(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_new_on_dev(args->dev, args->addrs[i]);
		if (!vblk)
			return ENOMEM;

		buf = nvm_buf_alloc(args->geo, args->geo.vblk_nbytes);
		if (!buf) {
			nvm_vblk_free(vblk);
			return ENOMEM;
		}

		err = nvm_vblk_read(vblk, buf, args->geo.vblk_nbytes);
		if (err)
			printf("FAILED: nvm_vblk_read err(%ld)\n", err);

		if (getenv("NVM_BUF_PR"))
			nvm_buf_pr(buf, args->geo.vblk_nbytes);

		free(buf);
		nvm_vblk_free(vblk);
	}

	return err ? 1 : 0;
}

int pread(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		NVM_VBLK vblk;
		void *buf;
		const size_t offset = args->geo.vpg_nbytes * args->addrs[i].g.pg;

		printf("** nvm_vblk_pread(...):\n");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_new_on_dev(args->dev, args->addrs[i]);
		if (!vblk)
			return ENOMEM;

		buf = nvm_buf_alloc(args->geo, args->geo.vpg_nbytes);
		if (!buf) {
			nvm_vblk_free(vblk);
			return ENOMEM;
		}
		nvm_buf_fill(buf, args->geo.vpg_nbytes);

		err = nvm_vblk_pread(vblk, buf, args->geo.vpg_nbytes, offset);
		if (err)
			printf("FAILED: nvm_vblk_pread err(%ld)\n", err);

		if (getenv("NVM_BUF_PR"))
			nvm_buf_pr(buf, args->geo.vpg_nbytes);

		free(buf);
		nvm_vblk_free(vblk);
	}

	return err ? 1 : 0;
}

int get(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		NVM_VBLK vblk;

		printf("** nvm_vblk_gets(...):");
		nvm_addr_pr(args->addrs[i]);
		
		vblk = nvm_vblk_new();
		if (!vblk)
			return ENOMEM;

		err = nvm_vblk_gets(vblk, args->dev, args->addrs[i].g.ch,
				    args->addrs[i].g.ch);
		if (err) {
			printf("FAILED: nvm_vblk_gets err(%ld)\n", err);
		} else {
			printf("GOT: ");
			nvm_vblk_pr(vblk);
		}

		nvm_vblk_free(vblk);
	}

	return err ? 1 : 0;
}

int put(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t err = 0;
	
	for (int i = 0; i < args->naddrs; ++i) {
		NVM_VBLK vblk;

		printf("** nvm_vblk_put(...): ");
		nvm_addr_pr(args->addrs[i]);

		vblk = nvm_vblk_new_on_dev(args->dev, args->addrs[i]);
		if (!vblk)
			return ENOMEM;

		err = nvm_vblk_put(vblk);
		if (err) {
			printf("FAILED: nvm_vblk_put err(%ld)\n", err);
		}

		nvm_vblk_free(vblk);
	}

	return err ? 1 : 0;
}

/// CLI boiler-plate

static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_PPALIST, 0x0},
	{"write", write, NVM_CLI_ARG_PPALIST, 0x0},
	{"read", read, NVM_CLI_ARG_PPALIST, 0x0},
	{"pwrite", pwrite, NVM_CLI_ARG_PPALIST, 0x0},
	{"pread", pread, NVM_CLI_ARG_PPALIST, 0x0},
	{"gets", get, NVM_CLI_ARG_CH_LUN, 0x0},
	{"put", put, NVM_CLI_ARG_PPALIST, 0x0},
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
