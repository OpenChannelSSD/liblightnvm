#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm_cli.h>

ssize_t _write(struct nvm_cli *cli, int with_meta)
{
	struct nvm_cli_cmd_args *args = &cli->args;
	const int pmode = cli->evars.pmode;
	struct nvm_ret ret = {0,0};
	ssize_t err = 0;

	int buf_nbytes = args->naddrs * args->geo->sector_nbytes;
	char *buf = NULL;
	int meta_tbytes = args->naddrs * args->geo->meta_nbytes;
	char *meta = NULL;

	buf = nvm_buf_alloc(args->geo, buf_nbytes);	// data buffer
	if (!buf) {
		errno = ENOMEM;
		return -1;
	}

	if ((cli->opts.mask & NVM_CLI_OPT_FILE_INPUT) &&
	     cli->opts.file_input) {	// Fill buf with content from file
		if (nvm_buf_from_file(buf, buf_nbytes, cli->opts.file_output)) {
			nvm_cli_perror("nvm_buf_from_file");
			free(buf);
			return -1;
		}
	} else {			// Fill buf with synthetic payload
		nvm_buf_fill(buf, buf_nbytes);
	}

	if (with_meta) {				// metadata buffer
		meta = nvm_buf_alloc(args->geo, meta_tbytes);
		if (!meta) {
			nvm_cli_perror("nvm_buf_alloc");
			free(buf);
			return -1;
		}
		for (int i = 0; i < meta_tbytes; ++i)
			meta[i] = (i / args->naddrs) % args->naddrs + 65;
	}

	nvm_cli_info_pr("nvm_addr_write : {pmode: %s}", nvm_pmode_str(pmode));
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_addr_write(args->dev, args->addrs, args->naddrs, buf, meta,
			     pmode, &ret);
	if (err) {
		nvm_cli_perror("nvm_addr_write");
		nvm_ret_pr(&ret);
	}

	free(buf);
	free(meta);

	return err;
}

ssize_t _read(struct nvm_cli *cli, int with_meta)
{
	struct nvm_cli_cmd_args *args = &cli->args;
	const int pmode = cli->evars.pmode;
	struct nvm_ret ret = {0,0};
	ssize_t err = 0;

	int buf_nbytes = args->naddrs * args->geo->sector_nbytes;
	char *buf = NULL;
	int meta_tbytes = args->naddrs * args->geo->meta_nbytes;
	char *meta = NULL;

	buf = nvm_buf_alloc(args->geo, buf_nbytes);	// data buffer
	if (!buf) {
		nvm_cli_perror("nvm_buf_alloc");
		return -1;
	}

	if (with_meta) {				// metadata buffer
		meta = nvm_buf_alloc(args->geo, meta_tbytes);
		if (!meta) {
			nvm_cli_perror("nvm_buf_alloc");
			free(buf);
			return -1;
		}
		memset(meta, 0, meta_tbytes);
	}

	nvm_cli_info_pr("nvm_addr_read: {pmode: %s}", nvm_pmode_str(pmode));
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	if (meta && cli->evars.meta_pr) {
		nvm_cli_info_pr("before_read: {meta_tbytes: %d}", meta_tbytes);
		nvm_buf_pr(meta, meta_tbytes);
	}

	err = nvm_addr_read(args->dev, args->addrs, args->naddrs, buf, meta,
			    pmode, &ret);
	if (err) {
		nvm_cli_perror("nvm_addr_read");
		nvm_ret_pr(&ret);
	}
	
	if (meta && cli->evars.meta_pr) {
		nvm_cli_info_pr("after_read: {meta_tbytes: %d}", meta_tbytes);
		nvm_buf_pr(meta, meta_tbytes);
	}

	if ((cli->opts.mask & NVM_CLI_OPT_FILE_OUTPUT) &&
	     cli->opts.file_output) {	// Write buffer to file system
		if (nvm_buf_to_file(buf, buf_nbytes, cli->opts.file_output))
			nvm_cli_perror("nvm_buf_to_file");
	}

	free(buf);
	free(meta);

	return err;
}

int erase(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;
	const int pmode = cli->evars.pmode;
	struct nvm_ret ret = {0,0};
	ssize_t err = 0;

	nvm_cli_info_pr("nvm_addr_erase: {pmode: %s}", nvm_pmode_str(pmode));
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_addr_erase(args->dev, args->addrs, args->naddrs, pmode,
			     &ret);
	if (err) {
		nvm_cli_perror("nvm_addr_erase");
		nvm_ret_pr(&ret);
	}

	return err ? 1 : 0;
}

int write(struct nvm_cli *cli)
{
	return _write(cli, 0) ? 1 : 0;
}

int write_wm(struct nvm_cli *cli)
{
	return _write(cli, 1) ? 1 : 0;
}

int read(struct nvm_cli *cli)
{
	return _read(cli, 0) ? 1 : 0;
}

int read_wm(struct nvm_cli *cli)
{
	return _read(cli, 1) ? 1 : 0;
}

int cmd_fmt(struct nvm_cli *cli)
{
	for (int i = 0; i < cli->args.naddrs; ++i)
		nvm_addr_pr(cli->args.addrs[i]);

	return 0;
}

int cmd_gen2dev(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen: "); nvm_addr_pr(args->addrs[i]);
		printf("dev: 0x%016lx\n", nvm_addr_gen2dev(args->dev, args->addrs[i]));
	}

	return 0;
}

int cmd_gen2lba(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen: "); nvm_addr_pr(args->addrs[i]);
		printf("lba: %064ld\n", nvm_addr_gen2lba(args->dev, args->addrs[i]));
	}

	return 0;
}

int cmd_gen2off(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->naddrs; ++i) {
		printf("gen: "); nvm_addr_pr(args->addrs[i]);
		printf("off: %064ld\n", nvm_addr_gen2off(args->dev, args->addrs[i]));
	}

	return 0;
}

int cmd_dev2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->nhex_vals; ++i) {
		printf("dev: %016lx\n", args->hex_vals[i]);
		printf("gen: "); nvm_addr_pr(nvm_addr_dev2gen(args->dev, args->hex_vals[i]));
	}

	return 0;
}

int cmd_lba2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->ndec_vals; ++i) {
		printf("lba: %064ld\n", args->dec_vals[i]);
		printf("gen: "); nvm_addr_pr(nvm_addr_lba2gen(args->dev, args->dec_vals[i]));
	}

	return 0;
}

int cmd_off2gen(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;

	for (int i = 0; i < args->ndec_vals; ++i) {
		printf("off: %064ld\n", args->dec_vals[i]);
		printf("gen: "); nvm_addr_pr(nvm_addr_off2gen(args->dev, args->dec_vals[i]));
	}

	return 0;
}

/**
 * Command-line interface (CLI) boiler-plate
 */

/* Define commands */
static struct nvm_cli_cmd cmds[] = {
	{"erase",	erase,		NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"write",	write,		NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
	{"write_wm",	write_wm,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
	{"read",	read,		NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
	{"read_wm",	read_wm,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
	{"from_geo",	cmd_fmt,	NVM_CLI_ARG_ADDR_SEC, NVM_CLI_OPT_DEFAULT},
	{"from_hex",	cmd_fmt,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"gen2dev",	cmd_gen2dev,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"gen2lba",	cmd_gen2lba,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"gen2off",	cmd_gen2off,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"dev2gen",	cmd_dev2gen,	NVM_CLI_ARG_HEXVAL_LIST, NVM_CLI_OPT_DEFAULT},
	{"lba2gen",	cmd_lba2gen,	NVM_CLI_ARG_DECVAL_LIST, NVM_CLI_OPT_DEFAULT},
	{"off2gen",	cmd_off2gen,	NVM_CLI_ARG_DECVAL_LIST, NVM_CLI_OPT_DEFAULT},
};

/* Define the CLI */
static struct nvm_cli cli = {
	.title = "NVM address (nvm_addr_*)",
	.descr_short = "Construct / convert addresses and perform vector IO",
	.cmds = cmds,
	.ncmds = sizeof(cmds) / sizeof(cmds[0]),
};

/* Initialize and run */
int main(int argc, char **argv)
{
	int res = 0;

	if (nvm_cli_init(&cli, argc, argv) < 0) {
		nvm_cli_perror("nvm_cli_init");
		return 1;
	}

	res = nvm_cli_run(&cli);

	nvm_cli_destroy(&cli);

	return res;
}

