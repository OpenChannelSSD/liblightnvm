#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_vblk *vblk;
		
		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return errno;
		}

		printf("** nvm_vblk_erase(...):\n");
		nvm_vblk_pr(vblk);

		nvm_timer_start();
		res = nvm_vblk_erase(vblk);
		if (res < 0)
			perror("nvm_vblk_erase");
		nvm_timer_stop();
		nvm_timer_pr("nvm_vblk_erase");

		nvm_vblk_free(vblk);
	}

	return res < 0;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		char *buf;
		struct nvm_vblk *vblk;
		const struct nvm_geo *geo;

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return errno;
		}
		geo = nvm_vblk_attr_geo(vblk);

		printf("** nvm_vblk_read(...):\n");
		nvm_vblk_pr(vblk);

		nvm_timer_start();
		buf = nvm_buf_alloc(geo, geo->tbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			return errno;
		}
		nvm_timer_stop();
		nvm_timer_pr("nvm_buf_alloc");

		nvm_timer_start();
		res = nvm_vblk_read(vblk, buf, geo->tbytes);
		if (res < 0)
			perror("nvm_vblk_read");
		nvm_timer_stop();
		nvm_timer_pr("nvm_vblk_read");

		free(buf);
		nvm_vblk_free(vblk);
	}

	return res < 0;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		char *buf;
		struct nvm_vblk *vblk;
		const struct nvm_geo *geo;

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return errno;
		}
		geo = nvm_vblk_attr_geo(vblk);

		printf("** nvm_vblk_write(...):\n");
		nvm_vblk_pr(vblk);

		nvm_timer_start();
		buf = nvm_buf_alloc(geo, geo->tbytes);
		if (!buf) {
			perror("nvm_buf_alloc");
			return errno;
		}
		nvm_timer_stop();
		nvm_timer_pr("nvm_buf_alloc");

		nvm_timer_start();
		res = nvm_vblk_write(vblk, buf, geo->tbytes);
		if (res < 0)
			perror("nvm_vblk_write");
		nvm_timer_stop();
		nvm_timer_pr("nvm_vblk_write");

		free(buf);
		nvm_vblk_free(vblk);
	}

	return res < 0;
}

int pad(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	for (int i = 0; i < args->naddrs; ++i) {
		struct nvm_vblk *vblk;

		vblk = nvm_vblk_alloc(args->dev, args->addrs[i]);
		if (!vblk) {
			perror("nvm_vblk_alloc");
			return errno;
		}

		printf("** nvm_vblk_pad(...):\n");
		nvm_vblk_pr(vblk);

		nvm_timer_start();
		res = nvm_vblk_pad(vblk);
		if (res < 0)
			perror("nvm_vblk_pad");
		nvm_timer_stop();
		nvm_timer_pr("nvm_vblk_pad");

		nvm_vblk_free(vblk);
	}

	return res < 0;
}

int span_erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	struct nvm_vblk *vblk;
	struct nvm_addr bgn, end;

	bgn = args->addrs[0];
	end = args->addrs[1];
	
	vblk = nvm_vblk_alloc_span(args->dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, bgn.g.blk);
	if (!vblk) {
		perror("nvm_vblk_alloc");
		return errno;
	}

	printf("** nvm_vblk_erase(...):\n");
	nvm_vblk_pr(vblk);

	nvm_timer_start();
	res = nvm_vblk_erase(vblk);
	if (res < 0)
		perror("nvm_vblk_erase");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_erase");

	nvm_vblk_free(vblk);

	return res < 0;
}

int span_read(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	struct nvm_addr bgn, end;
	struct nvm_vblk *vblk;
	const struct nvm_geo *geo;
	char *buf;

	bgn = args->addrs[0];
	end = args->addrs[1];

	vblk = nvm_vblk_alloc_span(args->dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, end.g.blk);
	if (!vblk) {
		perror("nvm_vblk_alloc");
		return errno;
	}
	geo = nvm_vblk_attr_geo(vblk);

	printf("** nvm_vblk_read(...):\n");
	nvm_vblk_pr(vblk);

	nvm_timer_start();
	buf = nvm_buf_alloc(geo, geo->tbytes);
	if (!buf) {
		perror("nvm_buf_alloc");
		return errno;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	res = nvm_vblk_read(vblk, buf, geo->tbytes);
	if (res < 0)
		perror("nvm_vblk_read");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_read");

	free(buf);
	nvm_vblk_free(vblk);

	return res < 0;
}

int span_write(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	struct nvm_addr bgn, end;
	struct nvm_vblk *vblk;
	const struct nvm_geo *geo;
	char *buf;

	bgn = args->addrs[0];
	end = args->addrs[1];

	vblk = nvm_vblk_alloc_span(args->dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, end.g.blk);
	if (!vblk) {
		perror("nvm_vblk_alloc");
		return errno;
	}
	geo = nvm_vblk_attr_geo(vblk);

	printf("** nvm_vblk_write(...):\n");
	nvm_vblk_pr(vblk);

	nvm_timer_start();
	buf = nvm_buf_alloc(geo, geo->tbytes);
	if (!buf) {
		perror("nvm_buf_alloc");
		return errno;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	res = nvm_vblk_write(vblk, buf, geo->tbytes);
	if (res < 0)
		perror("nvm_vblk_write");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_write");

	free(buf);
	nvm_vblk_free(vblk);

	return res < 0;
}

int span_pad(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res = 0;

	struct nvm_vblk *vblk;
	struct nvm_addr bgn, end;

	bgn = args->addrs[0];
	end = args->addrs[1];

	vblk = nvm_vblk_alloc_span(args->dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, end.g.blk);
	if (!vblk) {
		perror("nvm_vblk_alloc");
		return errno;
	}

	printf("** nvm_vblk_pad(...):\n");
	nvm_vblk_pr(vblk);

	nvm_timer_start();
	res = nvm_vblk_pad(vblk);
	if (res < 0)
		perror("nvm_vblk_pad");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_pad");

	nvm_vblk_free(vblk);

	return res < 0;
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_PPALIST, 0x0},
	{"read", read, NVM_CLI_ARG_PPALIST, 0x0},
	{"write", write, NVM_CLI_ARG_PPALIST, 0x0},
	{"pad", pad, NVM_CLI_ARG_PPALIST, 0x0},
	{"span_erase", span_erase, NVM_CLI_ARG_SPAN, 0x0},
	{"span_read", span_read, NVM_CLI_ARG_SPAN, 0x0},
	{"span_write", span_write, NVM_CLI_ARG_SPAN, 0x0},
	{"span_pad", span_pad, NVM_CLI_ARG_SPAN, 0x0},
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
		nvm_cli_usage(argv[0], "NVM Virtual Block (nvm_vblk_*)", cmds,
			      ncmds);
	}
	nvm_cli_teardown(cmd);

	return ret != 0;
}

