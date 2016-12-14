#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>
#include "nvm_cli.h"

int erase(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t res;

	printf("** nvm_sblk_erase(...): sblk_tbytes(%lu)\n",
	        args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);

	nvm_timer_start();
	res = nvm_sblk_erase(args->sblk);
	if (res < 0)
		perror("nvm_sblk_erase");
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_erase");

	return res < 0;
}

int write(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret;
	char *buf;

	printf("** nvm_sblk_write(...): sblk_tbytes(%lu)\n",
	       args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);
	
	nvm_timer_start();
	buf = nvm_buf_alloc(args->sblk_geo, args->sblk_geo.tbytes);
	if (!buf) {
		perror("nvm_buf_alloc");
		return errno;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	nvm_buf_fill(buf, args->sblk_geo.tbytes);
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_fill");

	nvm_timer_start();
	ret = nvm_sblk_write(args->sblk, buf, args->sblk_geo.tbytes);
	if (ret < 0)
		perror("nvm_sblk_write");
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_write");

	free(buf);

	return ret < 0;
}

int chunked_write(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret = 0;
	char *buf;
	size_t buf_sz = args->sblk_geo.nchannels * args->sblk_geo.nluns \
			* args->sblk_geo.nplanes * args->sblk_geo.nsectors \
			* args->sblk_geo.nbytes;
	size_t nbytes_written = 0;

	printf("** nvm_sblk_write(...): sblk_tbytes(%lu), buf_sz(%lu)\n",
	       args->sblk_geo.tbytes, buf_sz);
	nvm_sblk_pr(args->sblk);
	
	nvm_timer_start();
	buf = nvm_buf_alloc(args->sblk_geo, buf_sz);
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
	
	while(nbytes_written < args->sblk_geo.tbytes) {
		ret = nvm_sblk_write(args->sblk, buf, buf_sz);
		if (ret < 0) {
			perror("nvm_sblk_write");
			break;
		}
		nbytes_written += ret;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_write");

	free(buf);

	return ret < 0;
}

int pad(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret;

	printf("** nvm_sblk_pad(...): sblk_tbytes(%lu)\n",
	       args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);

	nvm_timer_start();
	ret = nvm_sblk_pad(args->sblk);
	if (ret < 0)
		perror("nvm_sblk_pad");
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_pad");

	return ret < 0;
}

int read(NVM_CLI_CMD_ARGS *args, int flags)
{
	ssize_t ret;
	char *buf;

	printf("** nvm_sblk_read(...): sblk_tbytes(%lu)\n",
	       args->sblk_geo.tbytes);
	nvm_sblk_pr(args->sblk);

	nvm_timer_start();
	buf = nvm_buf_alloc(args->sblk_geo, args->sblk_geo.tbytes);
	if (!buf) {
		perror("nvm_buf_alloc");
		return errno;
	}
	nvm_timer_stop();
	nvm_timer_pr("nvm_buf_alloc");

	nvm_timer_start();
	ret = nvm_sblk_read(args->sblk, buf, args->sblk_geo.tbytes);
	if (ret < 0)
		perror("nvm_sblk_read");
	nvm_timer_stop();
	nvm_timer_pr("nvm_sblk_read");

	free(buf);

	return ret < 0;
}

//
// Remaining code is CLI boiler-plate
//
static NVM_CLI_CMD cmds[] = {
	{"erase", erase, NVM_CLI_ARG_SBLK, 0x0},
	{"write", write, NVM_CLI_ARG_SBLK, 0x0},
	{"pad", pad, NVM_CLI_ARG_SBLK, 0x0},
	{"read", read, NVM_CLI_ARG_SBLK, 0x0},
	{"chunked_w", chunked_write, NVM_CLI_ARG_SBLK, 0x0},
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
		nvm_cli_usage(argv[0], "NVM spanning block (nvm_sblk_*)", cmds,
			      ncmds);
	}
	nvm_cli_teardown(cmd);

	return ret != 0;
}

