#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm_cli.h>

/*

//
// MOVED FROM "nvm_addr" CLI
//

static int cmd_addr_erase(struct nvm_cli *cli)
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

static int cmd_addr_write(struct nvm_cli *cli)
{
	return _write(cli, 0) ? 1 : 0;
}

static int cmd_addr_write_wm(struct nvm_cli *cli)
{
	return _write(cli, 1) ? 1 : 0;
}

static int cmd_addr_read(struct nvm_cli *cli)
{
	return _read(cli, 0) ? 1 : 0;
}

static int cmd_addr_read_wm(struct nvm_cli *cli)
{
	return _read(cli, 1) ? 1 : 0;
}
*/


static int cmd_idfy(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	struct nvm_spec_idfy *idfy = NULL;

	nvm_cli_info_pr("nvm_cmd_idfy");

	idfy = nvm_cmd_idfy(dev, NULL);
	if (!idfy)
		return -1;

	nvm_spec_idfy_pr(idfy);

	nvm_buf_free(idfy);

	return 0;
}

static int cmd_rprt(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	struct nvm_addr *addr = cli->args.naddrs ? &cli->args.addrs[0]: NULL;

	struct nvm_spec_rprt *rprt = NULL;

	nvm_cli_info_pr("nvm_cmd_rprt: %p", addr);
	if (addr)
		nvm_addr_pr(*addr);

	rprt = nvm_cmd_rprt(dev, addr, 0x0, NULL);
	if (!rprt)
		return -1;

	nvm_spec_rprt_pr(rprt);

	nvm_buf_free(rprt);

	return 0;
}

static int cmd_gbbt(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	struct nvm_spec_bbt *bbt = NULL;
	struct nvm_addr addr = cli->args.addrs[0];

	nvm_cli_info_pr("nvm_cmd_gbbt");

	bbt = nvm_cmd_gbbt(dev, addr, NULL);
	if (!bbt)
		return -1;

	nvm_spec_bbt_pr(bbt);

	nvm_buf_free(bbt);

	return 0;
}

static int cmd_sbbt(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	struct nvm_addr *addrs = cli->args.addrs;
	size_t naddrs = cli->args.naddrs;
	uint64_t flags = cli->args.hex_vals[0];

	nvm_cli_info_pr("nvm_cmd_sbbt");
	nvm_cli_info_pr("flags: 0x%04X", flags);

	nvm_addr_prn(addrs, naddrs, dev);

	if (nvm_cmd_sbbt(dev, addrs, naddrs, flags, NULL))
		nvm_cli_perror("nvm_cmd_sbbt");

	return 0;
}

static int cmd_erase(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;
	const int pmode = cli->evars.pmode;
	struct nvm_ret ret = {0,0};
	ssize_t err = 0;

	nvm_cli_info_pr("nvm_cmd_erase: {pmode: %s}", nvm_pmode_str(pmode));
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	err = nvm_cmd_erase(args->dev, args->addrs, args->naddrs, pmode,
			     &ret);
	if (err) {
		nvm_cli_perror("nvm_cmd_erase");
		nvm_ret_pr(&ret);
	}

	return err ? 1 : 0;
}

static int cmd_write(struct nvm_cli *cli)
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

	meta = nvm_buf_alloc(args->geo, meta_tbytes);
	if (!meta) {
		nvm_cli_perror("nvm_buf_alloc");
		nvm_buf_free(buf);
		return -1;
	}
	memset(meta, 0, meta_tbytes);

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
		for (int i = 0; i < meta_tbytes; ++i) {
			if (i)
				printf(", ");
			printf("0x%0x", meta[i]);
		}
	}

	if ((cli->opts.mask & NVM_CLI_OPT_FILE_OUTPUT) &&
	     cli->opts.file_output) {	// Write buffer to file system
		if (nvm_buf_to_file(buf, buf_nbytes, cli->opts.file_output))
			nvm_cli_perror("nvm_buf_to_file");
	}

	nvm_buf_free(buf);
	nvm_buf_free(meta);

	return err;
}

static int cmd_read(struct nvm_cli *cli)
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

	meta = nvm_buf_alloc(args->geo, meta_tbytes);
	if (!meta) {
		nvm_cli_perror("nvm_buf_alloc");
		nvm_buf_free(buf);
		return -1;
	}
	memset(meta, 0, meta_tbytes);

	nvm_cli_info_pr("nvm_addr_read: {pmode: %s}", nvm_pmode_str(pmode));
	for (int i = 0; i < args->naddrs; ++i) {
		nvm_addr_pr(args->addrs[i]);
	}

	if (meta && cli->evars.meta_pr) {
		nvm_cli_info_pr("before_read: {meta_tbytes: %d}", meta_tbytes);
		nvm_buf_pr(meta, meta_tbytes);
	}

	err = nvm_cmd_read(args->dev, args->addrs, args->naddrs, buf, meta,
			    pmode, &ret);
	if (err) {
		nvm_cli_perror("nvm_cmd_read");
		nvm_ret_pr(&ret);
	}
	
	if (meta && cli->evars.meta_pr) {
		nvm_cli_info_pr("after_read: {meta_tbytes: %d}", meta_tbytes);
		for (int i = 0; i < meta_tbytes; ++i) {
			if (i)
				printf(", ");
			printf("0x%0x", meta[i]);
		}
	}

	if ((cli->opts.mask & NVM_CLI_OPT_FILE_OUTPUT) &&
	     cli->opts.file_output) {	// Write buffer to file system
		if (nvm_buf_to_file(buf, buf_nbytes, cli->opts.file_output))
			nvm_cli_perror("nvm_buf_to_file");
	}

	nvm_buf_free(buf);
	nvm_buf_free(meta);

	return err;
}

static int cmd_copy(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	const uint32_t WS_MIN = nvm_dev_get_ws_min(dev);
	const size_t nsectr = cli->args.geo->l.nsectr;

	struct nvm_addr chunk_src = cli->args.addrs[0];
	struct nvm_addr chunk_dst = cli->args.addrs[1];

	nvm_cli_info_pr("nvm_cmd_copy");
	nvm_cli_info_pr("src");
	nvm_addr_prn(&chunk_src, 1, dev);

	nvm_cli_info_pr("dst");
	nvm_addr_prn(&chunk_dst, 1, dev);

	for (size_t sectr = 0; sectr < nsectr; sectr += WS_MIN) {
		struct nvm_addr src[WS_MIN];
		struct nvm_addr dst[WS_MIN];

		for (size_t idx = 0; idx < WS_MIN; ++idx) {
			src[idx].ppa = chunk_src.ppa;
			src[idx].l.sectr = sectr + idx;

			dst[idx].ppa = chunk_dst.ppa;
			dst[idx].l.sectr = sectr + idx;
		}

		if (nvm_cmd_copy(dev, src, dst, WS_MIN, 0x0, NULL)) {
			nvm_cli_info_pr("nvm_cmd_copy");
			nvm_cli_info_pr("src");
			nvm_addr_prn(src, WS_MIN, dev);
			nvm_cli_info_pr("dst");
			nvm_addr_prn(src, WS_MIN, dev);

			nvm_cli_perror("nvm_cmd_copy");
			break;
		}
	}

	return 0;
}

/**
 * Command-line interface (CLI) boiler-plate
 */

/* Define commands */
static struct nvm_cli_cmd cmds[] = {
	{"idfy",	cmd_idfy,	NVM_CLI_ARG_DEV_PATH, NVM_CLI_OPT_DEFAULT},
	{"rprt_all",	cmd_rprt,	NVM_CLI_ARG_DEV_PATH, NVM_CLI_OPT_DEFAULT},
	{"rprt_lun",	cmd_rprt,	NVM_CLI_ARG_ADDR_LUN, NVM_CLI_OPT_DEFAULT},
	{"gbbt",	cmd_gbbt,	NVM_CLI_ARG_ADDR_LUN, NVM_CLI_OPT_DEFAULT},
	{"sbbt",	cmd_sbbt,	NVM_CLI_ARG_ADDR_BLK_HEXVAL, NVM_CLI_OPT_DEFAULT},
	{"erase",	cmd_erase,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
	{"write",	cmd_write,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
	{"read",	cmd_read,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
	{"copy",	cmd_copy,	NVM_CLI_ARG_VCOPY_S20, NVM_CLI_OPT_DEFAULT},
/*
 
{"erase",	cmd_addr_erase,		NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT},
{"write",	cmd_addr_write,		NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
{"write_wm",	cmd_addr_write_wm,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_INPUT},
{"read",	cmd_addr_read,		NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
{"read_wm",	cmd_addr_read_wm,	NVM_CLI_ARG_ADDR_LIST, NVM_CLI_OPT_DEFAULT | NVM_CLI_OPT_FILE_OUTPUT},
*/

};


/* Define the CLI */
static struct nvm_cli cli = {
	.title = "NVM command (nvm_cmd_*)",
	.descr_short = "Construct and execute NVM commands",
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

