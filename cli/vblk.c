#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res;

	printf("** nvm_vblk_erase(...): vblk_tbytes(%lu)\n",
	        args->vblk_geo->tbytes);
	nvm_vblk_pr(args->vblk);

	nvm_timer_start();
	res = nvm_vblk_erase(args->vblk);
	if (res < 0)
		perror("nvm_vblk_erase");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_erase");

	return res < 0;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret;
	char *buf;

	printf("** nvm_vblk_write(...): vblk_tbytes(%lu)\n",
	       args->vblk_geo->tbytes);
	nvm_vblk_pr(args->vblk);
	
	nvm_timer_start();
	buf = nvm_buf_alloc(args->vblk_geo, args->vblk_geo->tbytes);
	if (!buf) {
		perror("nvm_buf_alloc");
		return errno;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	nvm_buf_fill(buf, args->vblk_geo->tbytes);
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_fill");

	nvm_timer_start();
	ret = nvm_vblk_write(args->vblk, buf, args->vblk_geo->tbytes);
	if (ret < 0)
		perror("nvm_vblk_write");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_write");

	free(buf);

	return ret < 0;
}

int write_chunked(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret = 0;
	char *buf;
	size_t buf_sz = args->vblk_geo->nchannels * args->vblk_geo->nluns \
			* args->vblk_geo->nplanes * args->vblk_geo->nsectors \
			* args->vblk_geo->sector_nbytes;
	size_t nbytes_written = 0;

	printf("** nvm_vblk_write(...): vblk_tbytes(%lu), buf_sz(%lu)\n",
	       args->vblk_geo->tbytes, buf_sz);
	nvm_vblk_pr(args->vblk);
	
	nvm_timer_start();
	buf = nvm_buf_alloc(args->vblk_geo, buf_sz);
	if (!buf) {
		perror("nvm_buf_alloc");
		return errno;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	nvm_buf_fill(buf, buf_sz);
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_fill");

	nvm_timer_start();
	
	while(nbytes_written < args->vblk_geo->tbytes) {
		ret = nvm_vblk_write(args->vblk, buf, buf_sz);
		if (ret < 0) {
			perror("nvm_vblk_write");
			break;
		}
		nbytes_written += ret;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_write");

	free(buf);

	return ret < 0;
}

int pad(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret;

	printf("** nvm_vblk_pad(...): vblk_tbytes(%lu)\n",
	       args->vblk_geo->tbytes);
	nvm_vblk_pr(args->vblk);

	nvm_timer_start();
	ret = nvm_vblk_pad(args->vblk);
	if (ret < 0)
		perror("nvm_vblk_pad");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_pad");

	return ret < 0;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret;
	char *buf;

	printf("** nvm_vblk_read(...): vblk_tbytes(%lu)\n",
	       args->vblk_geo->tbytes);
	nvm_vblk_pr(args->vblk);

	nvm_timer_start();
	buf = nvm_buf_alloc(args->vblk_geo, args->vblk_geo->tbytes);
	if (!buf) {
		perror("nvm_buf_alloc");
		return errno;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	ret = nvm_vblk_read(args->vblk, buf, args->vblk_geo->tbytes);
	if (ret < 0)
		perror("nvm_vblk_read");
	nvm_timer_stop();
	nvm_timer_pr("nvm_vblk_read");

	free(buf);

	return ret < 0;
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_PPALIST, 0x0},
	{"read", read, NVM_CLI_ARG_PPALIST, 0x0},
	{"write", write, NVM_CLI_ARG_PPALIST, 0x0},
	{"pad", pad, NVM_CLI_ARG_PPALIST, 0x0},
	{"span_erase", erase, NVM_CLI_ARG_SPAN, 0x0},
	{"span_read", read, NVM_CLI_ARG_SPAN, 0x0},
	{"span_write", write, NVM_CLI_ARG_SPAN, 0x0},
	{"span_pad", pad, NVM_CLI_ARG_SPAN, 0x0},
	{"write_chnk", write_chunked, NVM_CLI_ARG_SPAN, 0x0},
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

