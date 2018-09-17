#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm_cli.h>

static int cmd_idfy(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	struct nvm_spec_idfy *idfy = NULL;

	nvm_cli_info_pr("nvm_cmd_idfy");

	idfy = nvm_cmd_idfy(dev, NULL);
	if (!idfy)
		return -1;

	nvm_spec_idfy_pr(idfy, nvm_dev_get_quirks(dev));

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
		nvm_addr_prn(addr, 1, cli->args.dev);

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
	struct nvm_ret ret = { 0 };
	ssize_t err = 0;

	nvm_cli_info_pr("nvm_cmd_erase: {pmode: %s}", nvm_pmode_str(pmode));
	nvm_addr_prn(args->addrs, args->naddrs, args->dev);

	err = nvm_cmd_erase(args->dev, args->addrs, args->naddrs, NULL, pmode,
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
	struct nvm_ret ret = { 0 };
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
	if ((cli->opts.mask & NVM_CLI_OPT_FILE_INPUT) &&
	     cli->opts.file_input) {	// Fill with content from file
		if (nvm_buf_from_file(buf, buf_nbytes, cli->opts.file_input)) {
			nvm_cli_perror("nvm_buf_from_file");
			nvm_buf_free(buf);
			return -1;
		}
	} else {
		nvm_buf_fill(buf, buf_nbytes);	// Fill with synthetic payload
	}

	if (meta_tbytes) {
		meta = nvm_buf_alloc(args->geo, meta_tbytes);
		if (!meta) {
			nvm_cli_perror("nvm_buf_alloc");
			nvm_buf_free(buf);
			return -1;
		}
		memset(meta, 0, meta_tbytes);
	}

	nvm_cli_info_pr("nvm_cmd_write: {pmode: %s}", nvm_pmode_str(pmode));
	nvm_addr_prn(args->addrs, args->naddrs, args->dev);

	err = nvm_cmd_write(args->dev, args->addrs, args->naddrs, buf, meta,
			    pmode, &ret);
	if (err) {
		nvm_cli_perror("nvm_cmd_write");
		nvm_ret_pr(&ret);
	}

	nvm_buf_free(buf);
	nvm_buf_free(meta);

	return err;
}

static int cmd_read(struct nvm_cli *cli)
{
	struct nvm_cli_cmd_args *args = &cli->args;
	const int pmode = cli->evars.pmode;
	struct nvm_ret ret = { 0 };
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

	if (meta_tbytes) {
		meta = nvm_buf_alloc(args->geo, meta_tbytes);
		if (!meta) {
			nvm_cli_perror("nvm_buf_alloc");
			nvm_buf_free(buf);
			return -1;
		}
		memset(meta, 0, meta_tbytes);
	}

	nvm_cli_info_pr("nvm_cmd_read: {pmode: %s}", nvm_pmode_str(pmode));
	nvm_addr_prn(args->addrs, args->naddrs, args->dev);

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

/*
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
			src[idx].val = chunk_src.val;
			src[idx].l.sectr = sectr + idx;

			dst[idx].val = chunk_dst.val;
			dst[idx].l.sectr = sectr + idx;
		}

		if (nvm_cmd_copy(dev, src, dst, WS_MIN, 0x0, NULL)) {
			nvm_cli_info_pr("nvm_cmd_copy -- failed");
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
*/

static int cmd_copy(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);
	const uint32_t WS_MIN = nvm_dev_get_ws_min(dev);
	size_t nerr = 0;

	struct nvm_addr *src = &cli->args.addrs[0];
	struct nvm_addr *dst = &cli->args.addrs[1];

	const size_t nchunks = 1;
	const size_t offset = 0;
	const size_t count = geo->l.nbytes * geo->l.nsectr;

	const size_t sectr_nbytes = geo->l.nbytes;
	const size_t nsectr = count / sectr_nbytes;

	const size_t sectr_bgn = offset / sectr_nbytes;
	const size_t sectr_end = sectr_bgn + (count / sectr_nbytes) - 1;

	const size_t cmd_nsectr_max = (NVM_NADDR_MAX / WS_MIN) * WS_MIN;

	if (nsectr % WS_MIN) {
		errno = EINVAL;
		return -1;
	}
	if (sectr_bgn % WS_MIN) {
		errno = EINVAL;
		return -1;
	}

	for (size_t sectr_ofz = sectr_bgn; sectr_ofz <= sectr_end; sectr_ofz += cmd_nsectr_max) {
		struct nvm_ret ret = { 0 };

		const size_t cmd_nsectr = NVM_MIN(sectr_end - sectr_ofz + 1, cmd_nsectr_max);

		struct nvm_addr addrs_src[cmd_nsectr];
		struct nvm_addr addrs_dst[cmd_nsectr];

		for (size_t idx = 0; idx < cmd_nsectr; ++idx) {
			const size_t sectr = sectr_ofz + idx;
			const size_t wunit = sectr / WS_MIN;
			const size_t rnd = wunit / nchunks;

			const size_t chunk_sectr = sectr % WS_MIN + rnd * WS_MIN;

			addrs_src[idx].val = src->val;
			addrs_src[idx].l.sectr = chunk_sectr;

			addrs_dst[idx].val = dst->val;
			addrs_dst[idx].l.sectr = chunk_sectr;
		}

		const ssize_t err = nvm_cmd_copy(dev, addrs_src,
						 addrs_dst, cmd_nsectr, 0x0,
						 &ret);
		if (err)
			++nerr;
	}

	if (nerr) {
		errno = EIO;
		return -1;
	}

	return count;

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
