#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm_cli.h>
#include <nvm_utils.h>

ssize_t _vblk_erase(struct nvm_cli *NVM_UNUSED(cli), struct nvm_vblk *vblk)
{
	ssize_t res = 0;

	nvm_vblk_pr(vblk);

	nvm_cli_timer_start();
	res = nvm_vblk_erase(vblk);
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_vblk_erase", nvm_vblk_get_nbytes(vblk));

	if (res < 0)
		nvm_cli_perror("nvm_vblk_erase");

	return res;
}

ssize_t _vblk_write(struct nvm_cli *cli, struct nvm_vblk *vblk)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(cli->args.dev);
	const size_t nbytes = nvm_vblk_get_nbytes(vblk);
	char *buf = NULL;
	ssize_t res = 0;

	nvm_vblk_pr(vblk);

	nvm_cli_timer_start();		// Allocate write buffer
	buf = nvm_buf_alloc(geo, nbytes);
	if (!buf) {
		nvm_cli_perror("nvm_buf_alloc");
		return -1;
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_pr("nvm_buf_alloc");

	nvm_cli_timer_start();		// Allocate write buffer
	if ((cli->opts.mask & NVM_CLI_OPT_FILE_INPUT) &&
	     cli->opts.file_input) {	// Fill with content from file
		if (nvm_buf_from_file(buf, nbytes, cli->opts.file_input)) {
			nvm_cli_perror("nvm_buf_from_file");
			free(buf);
			return -1;
		}
	} else {
		nvm_buf_fill(buf, nbytes);	// Fill with synthetic payload
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_pr("nvm_buf_fill");

	nvm_cli_timer_start();
	res = nvm_vblk_write(vblk, buf, nbytes);// Write buffer to device
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_vblk_write", nbytes);

	if (res < 0)
		nvm_cli_perror("nvm_vblk_write");

	free(buf);

	return res;
}

ssize_t _vblk_read(struct nvm_cli *cli, struct nvm_vblk *vblk)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(cli->args.dev);
	const size_t nbytes = nvm_vblk_get_nbytes(vblk);
	char *buf = NULL;
	ssize_t res = 0;

	nvm_vblk_pr(vblk);

	nvm_cli_timer_start();		// Allocate read buffer
	buf = nvm_buf_alloc(geo, nbytes);
	if (!buf) {
		nvm_cli_perror("nvm_buf_alloc");
		return -1;
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_pr("nvm_buf_alloc");

	nvm_cli_timer_start();		// Allocate write buffer
	nvm_buf_fill(buf, nbytes);	// Fill with synthetic payload
	nvm_cli_timer_stop();
	nvm_cli_timer_pr("nvm_buf_fill");

	nvm_cli_timer_start();
	res = nvm_vblk_read(vblk, buf, nbytes);	// READ: DEV -> BUFFER
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_vblk_read", nbytes);

	if (res < 0)
		nvm_cli_perror("nvm_vblk_read");

	if ((cli->opts.mask & NVM_CLI_OPT_FILE_OUTPUT) &&
	     cli->opts.file_output) {	// Write buffer to file system
		if (nvm_buf_to_file(buf, nbytes, cli->opts.file_output))
			nvm_cli_perror("nvm_buf_to_file");
	}

	free(buf);

	return res;
}

ssize_t _vblk_pread(struct nvm_cli *cli, struct nvm_vblk *vblk)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(cli->args.dev);
	char *buf = NULL;
	ssize_t res = 0;
	size_t nbytes = cli->args.dec_vals[0];
	size_t offset = cli->args.dec_vals[1];

	nvm_vblk_pr(vblk);

	nvm_cli_timer_start();		// Allocate read buffer
	buf = nvm_buf_alloc(geo, nbytes);
	if (!buf) {
		nvm_cli_perror("nvm_buf_alloc");
		return -1;
	}
	nvm_cli_timer_stop();
	nvm_cli_timer_pr("nvm_buf_alloc");

	nvm_cli_timer_start();		// Read from device into buffer
	res = nvm_vblk_pread(vblk, buf, nbytes, offset);
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_vblk_pread", nbytes);

	if (res < 0)
		nvm_cli_perror("nvm_vblk_pread");

	if ((cli->opts.mask & NVM_CLI_OPT_FILE_OUTPUT) &&
	     cli->opts.file_output) {	// Write buffer to file system
		if (nvm_buf_to_file(buf, nbytes, cli->opts.file_output))
			nvm_cli_perror("nvm_buf_to_file");
	}

	free(buf);

	return res;
}

ssize_t _vblk_pad(struct nvm_cli *NVM_UNUSED(cli), struct nvm_vblk *vblk)
{
	ssize_t res = 0;

	nvm_vblk_pr(vblk);

	nvm_cli_timer_start();		// Read from device into buffer
	res = nvm_vblk_pad(vblk);
	nvm_cli_timer_stop();
	nvm_cli_timer_bw_pr("nvm_vblk_pad", nvm_vblk_get_nbytes(vblk));

	if (res < 0)
		nvm_cli_perror("nvm_vblk_pad");

	return res;
}

/**
 * Construct plane-spanning block
 */
int erase(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc(cli->args.dev, &cli->args.addrs[0], 1);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_erase(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int write(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;

	vblk = nvm_vblk_alloc(cli->args.dev, &cli->args.addrs[0], 1);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_write(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int read(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;

	vblk = nvm_vblk_alloc(cli->args.dev, &cli->args.addrs[0], 1);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_read(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int pad(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;

	vblk = nvm_vblk_alloc(cli->args.dev, &cli->args.addrs[0], 1);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_read(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int line_erase(struct nvm_cli *cli)
{
	struct nvm_addr bgn = cli->args.addrs[0],
			end = cli->args.addrs[1];
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc_line(cli->args.dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, bgn.g.blk);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_erase(cli, vblk);
	
	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int line_write(struct nvm_cli *cli)
{
	struct nvm_addr bgn = cli->args.addrs[0],
			end = cli->args.addrs[1];
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc_line(cli->args.dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, bgn.g.blk);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_write(cli, vblk);
	
	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int line_read(struct nvm_cli *cli)
{
	struct nvm_addr bgn = cli->args.addrs[0],
			end = cli->args.addrs[1];
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc_line(cli->args.dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, bgn.g.blk);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_read(cli, vblk);
	
	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int line_pread(struct nvm_cli *cli)
{
	struct nvm_addr bgn = cli->args.addrs[0],
			end = cli->args.addrs[1];
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc_line(cli->args.dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, bgn.g.blk);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_pread(cli, vblk);
	
	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int line_pad(struct nvm_cli *cli)
{
	struct nvm_addr bgn = cli->args.addrs[0],
			end = cli->args.addrs[1];
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc_line(cli->args.dev, bgn.g.ch, end.g.ch, bgn.g.lun,
				   end.g.lun, bgn.g.blk);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_pad(cli, vblk);
	
	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int set_erase(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc(cli->args.dev, cli->args.addrs, cli->args.naddrs);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return -1;
	}

	res = _vblk_erase(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int set_write(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc(cli->args.dev, cli->args.addrs, cli->args.naddrs);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return errno;
	}

	res = _vblk_write(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int set_read(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc(cli->args.dev, cli->args.addrs, cli->args.naddrs);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return errno;
	}

	res = _vblk_read(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

int set_pad(struct nvm_cli *cli)
{
	struct nvm_vblk *vblk = NULL;
	ssize_t res = 0;
	
	vblk = nvm_vblk_alloc(cli->args.dev, cli->args.addrs, cli->args.naddrs);
	if (!vblk) {
		nvm_cli_perror("nvm_vblk_alloc");
		return errno;
	}

	res = _vblk_pad(cli, vblk);

	nvm_vblk_free(vblk);

	return res < 0 ? -1 : 0;
}

//
// Remaining code is CLI boiler-plate
//
static struct nvm_cli_cmd cmds[] = {
	{"erase",	erase,	NVM_CLI_ARG_ADDR,	NVM_CLI_OPT_DEFAULT},
	{"write",	write,	NVM_CLI_ARG_ADDR,	NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
	{"read",	read,	NVM_CLI_ARG_ADDR,	NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
	{"pad",		pad,	NVM_CLI_ARG_ADDR,	NVM_CLI_OPT_DEFAULT},

	{"set_erase",	set_erase,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT},
	{"set_write",	set_write,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
	{"set_read",	set_read,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
	{"set_pad",	set_pad,	NVM_CLI_ARG_ADDR_LIST,	NVM_CLI_OPT_DEFAULT},

	{"line_erase",	line_erase,	NVM_CLI_ARG_VBLK_LINE,	NVM_CLI_OPT_DEFAULT},
	{"line_write",	line_write,	NVM_CLI_ARG_VBLK_LINE,	NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
	{"line_read",	line_read,	NVM_CLI_ARG_VBLK_LINE,	NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
	{"line_pread",	line_pread,	NVM_CLI_ARG_VBLK_LINE_POS,	NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
	{"line_pad",	line_pad,	NVM_CLI_ARG_VBLK_LINE,	NVM_CLI_OPT_DEFAULT},
};

/* Define the CLI */
static struct nvm_cli cli = {
	.title = "NVM Virtual Block (nvm_vblk_*)",
	.descr_short = "Erase, write, and read virtual blocks",
	.cmds = cmds,
	.ncmds = sizeof(cmds) / sizeof(cmds[0]),
};

/* Initialize and run */
int main(int argc, char **argv)
{
	int res = 0;

	if (nvm_cli_init(&cli, argc, argv) < 0) {
		nvm_cli_perror("FAILED");
		return 1;
	}

	res = nvm_cli_run(&cli);

	nvm_cli_destroy(&cli);

	return res;
}
