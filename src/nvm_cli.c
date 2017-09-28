/*
 * nvm_cli - Command-line interface (CLI) utitities for liblightnvm
 *
 * Copyright (C) 2015-2017 Javier Gonzáles <javier@cnexlabs.com>
 * Copyright (C) 2015-2017 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <liblightnvm_cli.h>

static size_t start, stop;

static inline size_t _clock_sample(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);

	return ts.tv_nsec + ts.tv_sec * 1e9;
}

size_t nvm_cli_timer_start(void)
{
	start = _clock_sample();
	return start;
}

size_t nvm_cli_timer_stop(void)
{
	stop = _clock_sample();
	return stop;
}

double nvm_cli_timer_elapsed(void)
{
	return nvm_cli_timer_elapsed_secs();
}

double nvm_cli_timer_elapsed_secs(void)
{
	return (stop - start) / (double)1e9;
}

double nvm_cli_timer_elapsed_msecs(void)
{
	return (stop - start) / (double)1e6;
}

double nvm_cli_timer_elapsed_usecs(void)
{
	return (stop - start) / (double)1e3;
}

size_t nvm_cli_timer_elapsed_nsecs(void)
{
	return stop - start;
}

void nvm_cli_timer_pr(const char *tool)
{
	printf("%s: {elapsed: %lf}\n", tool, nvm_cli_timer_elapsed());
}

void nvm_cli_timer_bw_pr(const char *prefix, size_t nbytes)
{
	double secs = nvm_cli_timer_elapsed_secs();
	double mb = nbytes / (double)1048576;

	printf("%s: {elapsed: %.4f, mb: %.2f, mbsec: %.2f}\n",
		prefix, secs, mb, mb / secs);
}

int _parse_options(int argc, char *argv[], struct nvm_cli *cli)
{
	for (int opt = 0; (opt = getopt(argc, argv, ":hbvi:o:n:x:")) != -1;) {
		switch(opt) {
		case 'h':
			cli->opts.mask |= NVM_CLI_OPT_HELP;
			cli->opts.help = 1;
			break;
		case 'b':
			cli->opts.mask |= NVM_CLI_OPT_BRIEF;
			cli->opts.brief = 1;
			break;
		case 'v':
			cli->opts.mask |= NVM_CLI_OPT_VERBOSE;
			cli->opts.verbose = 1;
			break;
		case 's':
			cli->opts.mask |= NVM_CLI_OPT_STATUS;
			cli->opts.status = 1;
			break;
		case 'i':
			cli->opts.mask |= NVM_CLI_OPT_FILE_INPUT;
			cli->opts.file_input = optarg;
			break;
		case 'o':
			cli->opts.mask |= NVM_CLI_OPT_FILE_OUTPUT;
			cli->opts.file_output = optarg;
			break;
		case 'n':
			cli->opts.mask |= NVM_CLI_OPT_VAL_DEC;
			cli->opts.dec_val = atoi(optarg);
			break;
		case 'x':
			cli->opts.mask |= NVM_CLI_OPT_VAL_HEX;
			cli->opts.hex_val = strtoll(optarg, NULL, 16);
			break;

		case ':':
		case '?':
			return -1;
		default:
			break;
		}
	}

	return optind;
}

int _parse_cmd_arg_count_offset(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 2;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.dec_vals[0] = atoi(argv[0]);
	cli->args.dec_vals[1] = atoi(argv[1]);
	cli->args.ndec_vals = 2;

	return inc;
}

int _parse_cmd_arg_vcopy(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 6;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[1]);
	cli->args.addrs[0].g.blk = atoi(argv[2]);

	cli->args.addrs[1].ppa = 0;
	cli->args.addrs[1].g.ch = atoi(argv[3]);
	cli->args.addrs[1].g.lun = atoi(argv[4]);
	cli->args.addrs[1].g.blk = atoi(argv[5]);

	cli->args.naddrs = 2;

	return inc;
}

int _parse_cmd_arg_vblk_line(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 5;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[2]);
	cli->args.addrs[0].g.blk = atoi(argv[4]);

	cli->args.addrs[1].ppa = 0;
	cli->args.addrs[1].g.ch = atoi(argv[1]);
	cli->args.addrs[1].g.lun = atoi(argv[3]);
	cli->args.addrs[1].g.blk = atoi(argv[4]);
	cli->args.naddrs = 2;

	return inc;
}

int _parse_cmd_arg_vblk_line_pos(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 7;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[2]);
	cli->args.addrs[0].g.blk = atoi(argv[4]);

	cli->args.addrs[1].ppa = 0;
	cli->args.addrs[1].g.ch = atoi(argv[1]);
	cli->args.addrs[1].g.lun = atoi(argv[3]);
	cli->args.addrs[1].g.blk = atoi(argv[4]);
	cli->args.naddrs = 2;

	cli->args.dec_vals[0] = atol(argv[5]);
	cli->args.dec_vals[1] = atol(argv[6]);
	cli->args.ndec_vals = 2;

	return inc;
}

int _parse_cmd_arg_addr_sec(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 6;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[1]);
	cli->args.addrs[0].g.pl = atoi(argv[2]);
	cli->args.addrs[0].g.blk = atoi(argv[3]);
	cli->args.addrs[0].g.pg = atoi(argv[4]);
	cli->args.addrs[0].g.sec = atoi(argv[5]);
	cli->args.naddrs = 1;

	return inc;
}

int _parse_cmd_arg_addr_lun_hexval(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 3;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[1]);
	cli->args.naddrs = 1;

	cli->args.hex_vals[0] = strtoll(argv[2], NULL, 16);
	cli->args.nhex_vals = 1;

	return inc;
}

int _parse_cmd_arg_addr_blk_hexval(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 4;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[1]);
	cli->args.addrs[0].g.blk = atoi(argv[2]);
	cli->args.naddrs = 1;

	cli->args.hex_vals[0] = strtoll(argv[3], NULL, 16);
	cli->args.nhex_vals = 1;

	return inc;
}

int _parse_cmd_arg_addr_chk_hexval(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 4;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].l.pugrp = atoi(argv[0]);
	cli->args.addrs[0].l.punit = atoi(argv[1]);
	cli->args.addrs[0].l.chunk = atoi(argv[2]);
	cli->args.naddrs = 1;

	cli->args.hex_vals[0] = strtoll(argv[3], NULL, 16);
	cli->args.nhex_vals = 1;

	return inc;
}

int _parse_cmd_arg_addr_chk_val_hexval(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 5;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].l.pugrp = atoi(argv[0]);
	cli->args.addrs[0].l.punit = atoi(argv[1]);
	cli->args.addrs[0].l.chunk = atoi(argv[2]);
	cli->args.naddrs = 1;

	cli->args.dec_vals[0] = strtoll(argv[3], NULL, 10);
	cli->args.ndec_vals = 1;

	cli->args.hex_vals[0] = strtoll(argv[4], NULL, 16);
	cli->args.nhex_vals = 1;

	return inc;
}

int _parse_cmd_arg_addr_pg(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 4;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[1]);
	cli->args.addrs[0].g.blk = atoi(argv[2]);
	cli->args.addrs[0].g.pg = atoi(argv[3]);
	cli->args.naddrs = 1;

	return inc;
}

int _parse_cmd_arg_addr_blk(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 3;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[1]);
	cli->args.addrs[0].g.blk = atoi(argv[2]);
	cli->args.naddrs = 1;

	return inc;
}

int _parse_cmd_arg_addr_lun(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 2;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[0].ppa = 0;
	cli->args.addrs[0].g.ch = atoi(argv[0]);
	cli->args.addrs[0].g.lun = atoi(argv[1]);
	cli->args.naddrs = 1;

	return inc;
}

int _parse_cmd_arg_register(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 1;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.hex_vals[(cli->args.nhex_vals)++] = strtoll(argv[0], NULL, 16);

	return inc;
}

int _parse_cmd_arg_register_value(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 2;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.hex_vals[(cli->args.nhex_vals)++] = strtoll(argv[0], NULL, 16);
	cli->args.hex_vals[(cli->args.nhex_vals)++] = strtoll(argv[1], NULL, 16);

	return 1;
}

int _parse_cmd_arg_addr(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 1;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	cli->args.addrs[(cli->args.naddrs)++].ppa = strtoll(argv[0], NULL, 16);

	return inc;
}

int _parse_cmd_arg_addr_list(int argc, char *argv[], struct nvm_cli *cli)
{
	int inc = 0;
	
	for (int i = 0; (i < argc) && (argv[i][0] != '-'); ++i, ++inc) {
		cli->args.addrs[i].ppa = strtoll(argv[i], NULL, 16);
	}
	cli->args.naddrs = inc;

	return inc;
}

int _parse_cmd_arg_decval(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 1;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}
	
	cli->args.dec_vals[(cli->args.ndec_vals)++] = atoi(argv[0]);

	return inc;
}

int _parse_cmd_arg_decval_list(int argc, char *argv[], struct nvm_cli *cli)
{
	int inc = 0;
	
	for (int i = 0; (i < argc) && (argv[i][0] != '-'); ++i, ++inc)
		cli->args.dec_vals[i] = atoi(argv[i]);

	cli->args.ndec_vals = inc;

	return inc;
}

int _parse_cmd_arg_decval_begin_end(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 2;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}
	
	cli->args.dec_vals[0] = atoi(argv[0]);
	cli->args.dec_vals[1] = atoi(argv[1]);

	cli->args.ndec_vals = inc;

	return inc;
}


int _parse_cmd_arg_hexval(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 1;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}
	
	cli->args.hex_vals[(cli->args.nhex_vals)++] = strtoll(argv[0], NULL, 16);

	return inc;
}

int _parse_cmd_arg_hexval_list(int argc, char *argv[], struct nvm_cli *cli)
{
	int inc = 0;
	
	for (int i = 0; (i < argc) && (argv[i][0] != '-'); ++i, ++inc)
		cli->args.hex_vals[i] = strtoll(argv[i], NULL, 16);

	cli->args.nhex_vals = inc;

	return inc;
}

int _parse_cmd_arg_dev_path(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 1;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	if (strlen(argv[0]) < (size_t)inc ||
	    strlen(argv[0]) > NVM_DEV_PATH_LEN) {
		errno = EINVAL;
		return -1;
	}

	strcpy(cli->args.dev_path, argv[0]);

	return inc;
}

int _parse_cmd_args(int argc, char *argv[], struct nvm_cli *cli)
{
	int inc = 0;
	int ret;

	// Currently all commands except (ARG_NONE) take dev_path
	if (cli->cmd.arg_type != NVM_CLI_ARG_NONE) {
		ret = _parse_cmd_arg_dev_path(argc, argv, cli);	
		if (ret < 0)
			return -1;
		inc += ret;
	}

	// Invoke the remaining parsers
	switch (cli->cmd.arg_type) {
	case NVM_CLI_ARG_DECVAL:
		ret = _parse_cmd_arg_decval(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_DECVAL_LIST:
		ret = _parse_cmd_arg_decval_list(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_DECVAL_BEGIN_END:
		ret = _parse_cmd_arg_decval_begin_end(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_HEXVAL:
		ret = _parse_cmd_arg_hexval(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_HEXVAL_LIST:
		ret = _parse_cmd_arg_hexval_list(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR:
		ret = _parse_cmd_arg_addr(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_LIST:
		ret = _parse_cmd_arg_addr_list(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_LUN:
		ret = _parse_cmd_arg_addr_lun(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_BLK:
		ret = _parse_cmd_arg_addr_blk(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_PG:
		ret = _parse_cmd_arg_addr_pg(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_SEC:
		ret = _parse_cmd_arg_addr_sec(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_LUN_HEXVAL:
		ret = _parse_cmd_arg_addr_lun_hexval(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_BLK_HEXVAL:
		ret = _parse_cmd_arg_addr_blk_hexval(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_CHK_HEXVAL:
		ret = _parse_cmd_arg_addr_chk_hexval(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_ADDR_CHK_VAL_HEXVAL:
		ret = _parse_cmd_arg_addr_chk_val_hexval(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_VCOPY:
		ret = _parse_cmd_arg_vcopy(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_VBLK_LINE:
		ret = _parse_cmd_arg_vblk_line(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_VBLK_LINE_POS:
		ret = _parse_cmd_arg_vblk_line_pos(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_COUNT_OFFSET:
		ret = _parse_cmd_arg_count_offset(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_REGISTER:
		ret = _parse_cmd_arg_register(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_REGISTER_VALUE:
		ret = _parse_cmd_arg_register_value(argc - inc, argv + inc, cli);
		if (ret < 0)
			return -1;
		return inc + ret;

	case NVM_CLI_ARG_DEV_PATH:
	case NVM_CLI_ARG_NONE:
		return inc;
	}

	return -1;
}

int _parse_cmd(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 1;
	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	for (int i = 0; i < cli->ncmds; ++i)
		if (strcmp(argv[0], cli->cmds[i].name) == 0) {
			cli->cmd = cli->cmds[i];
			return inc;
		}

	errno = EINVAL;
	return -1;
}

int _parse_cli_name(int argc, char *argv[], struct nvm_cli *cli)
{
	const int inc = 1;

	if (argc < inc) {
		errno = EINVAL;
		return -1;
	}

	strcpy(cli->name, argv[0]);

	return inc;
}

/**
 * Environment variable parsers
 */

int _evar_pmode(struct nvm_cli *cli)
{
	struct nvm_dev *dev = cli->args.dev;
	const struct nvm_geo *geo = cli->args.geo;

	char *pmode_env = getenv("NVM_CLI_PMODE");
	if (!pmode_env) {
		cli->evars.pmode = nvm_dev_get_pmode(dev);
		return 0;
	}

	switch(strtoll(pmode_env, NULL, 16)) {
	case NVM_FLAG_PMODE_QUAD:
		if (geo->nplanes < 4) {	// Verify
			errno = EINVAL;
			return -1;
		}
		cli->evars.pmode = NVM_FLAG_PMODE_QUAD;
		return 0;
	case NVM_FLAG_PMODE_DUAL:
		if (geo->nplanes < 2) {	// Verify
			errno = EINVAL;
			return -1;
		}
		cli->evars.pmode = NVM_FLAG_PMODE_DUAL;
		return 0;
	case NVM_FLAG_PMODE_SNGL:
		cli->evars.pmode = NVM_FLAG_PMODE_SNGL;
		return 0;
	}

	errno = EINVAL;
	return -1;
}

int _evar_noverify(struct nvm_cli *cli)
{
	cli->evars.noverify = getenv("NVM_CLI_NOVERIFY") ? 1 : 0;

	return 0;
}

int _evar_meta_pr(struct nvm_cli *cli)
{
	cli->evars.meta_pr = getenv("NVM_CLI_META_PR") ? 1 : 0;

	return 0;
}

int _evar_erase_naddrs_max(struct nvm_cli *cli)
{
	char *erase_naddrs_max = getenv("NVM_CLI_ERASE_NADDRS_MAX");
	if (!erase_naddrs_max) {
		cli->evars.erase_naddrs_max = nvm_dev_get_erase_naddrs_max(
								cli->args.dev);
		return 0;
	}

	cli->evars.erase_naddrs_max = atoi(erase_naddrs_max);

	return 0;
}

int _evar_write_naddrs_max(struct nvm_cli *cli)
{
	char *write_naddrs_max = getenv("NVM_CLI_WRITE_NADDRS_MAX");
	if (!write_naddrs_max) {
		cli->evars.write_naddrs_max = nvm_dev_get_write_naddrs_max(
								cli->args.dev);
		return 0;
	}

	cli->evars.write_naddrs_max = atoi(write_naddrs_max);

	return 0;
}

int _evar_read_naddrs_max(struct nvm_cli *cli)
{
	char *read_naddrs_max = getenv("NVM_CLI_READ_NADDRS_MAX");
	if (!read_naddrs_max) {
		cli->evars.read_naddrs_max = nvm_dev_get_read_naddrs_max(
								cli->args.dev);
		return 0;
	}

	cli->evars.read_naddrs_max = atoi(read_naddrs_max);

	return 0;
}

int _evar_meta_mode(struct nvm_cli *cli)
{
	char *meta_mode_env = getenv("NVM_CLI_META_MODE");
	if (!meta_mode_env) {
		cli->evars.meta_mode = nvm_dev_get_meta_mode(cli->args.dev);
		return 0;
	}

	switch(strtoll(meta_mode_env, NULL, 16)) {
	case NVM_META_MODE_NONE:
		cli->evars.meta_mode = NVM_META_MODE_NONE;
		return 0;
	case NVM_META_MODE_ALPHA:
		cli->evars.meta_mode = NVM_META_MODE_ALPHA;
		return 0;
	case NVM_META_MODE_CONST:
		cli->evars.meta_mode = NVM_META_MODE_CONST;
		return 0;
	}

	errno = EINVAL;
	return -1;
}

int _evar_be_id(struct nvm_cli *cli)
{
	char *id = getenv("NVM_CLI_BE_ID");
	if (!id) {
		cli->evars.be_id = NVM_BE_ANY;
		return 0;
	}

	switch(strtoll(id, NULL, 16)) {
	case NVM_BE_ANY:
		cli->evars.be_id = NVM_BE_ANY;
		return 0;
	case NVM_BE_IOCTL:
		cli->evars.be_id = NVM_BE_IOCTL;
		return 0;
	case NVM_BE_SYSFS:
		cli->evars.be_id = NVM_BE_SYSFS;
		return 0;
	case NVM_BE_LBA:
		cli->evars.be_id = NVM_BE_LBA;
		return 0;
	case NVM_BE_SPDK:
		cli->evars.be_id = NVM_BE_SPDK;
		return 0;
	}

	errno = EINVAL;
	return -1;
}

int _evar_and_dev_setup(struct nvm_cli *cli)
{
	if (!cli) {
		errno = EINVAL;
		return -1;
	}

	if (_evar_be_id(cli) < 0) {		// Backend identifier
		perror("# NVM_CLI_BE_ID");
		return -1;
	}

	if (cli->cmd.arg_type == NVM_CLI_ARG_NONE) {
		return 0;
	}

	cli->args.dev = nvm_dev_openf(cli->args.dev_path, cli->evars.be_id);
	if (!cli->args.dev) {
		perror("# nvm_dev_openf");
		return -1;
	}

	cli->args.geo = nvm_dev_get_geo(cli->args.dev);

	if ((_evar_pmode(cli) < 0) ||
	    nvm_dev_set_pmode(cli->args.dev, cli->evars.pmode)) {
		perror("# NVM_CLI_PMODE");
		return -1;
	}

	if ((_evar_meta_mode(cli) < 0) ||
	    nvm_dev_set_meta_mode(cli->args.dev, cli->evars.meta_mode)) {
		perror("# NVM_CLI_META_MODE");
		return -1;
	}

	if ((_evar_erase_naddrs_max(cli) < 0) ||
	    nvm_dev_set_erase_naddrs_max(cli->args.dev,
					 cli->evars.erase_naddrs_max)) {
		perror("# NVM_CLI_ERASE_NADDRS_MAX");
		return -1;
	}
	if ((_evar_write_naddrs_max(cli) < 0) ||
	    nvm_dev_set_write_naddrs_max(cli->args.dev,
					 cli->evars.write_naddrs_max)) {
		perror("# NVM_CLI_WRITE_NADDRS_MAX");
		return -1;
	}
	if ((_evar_read_naddrs_max(cli) < 0) ||
	    nvm_dev_set_read_naddrs_max(cli->args.dev,
					 cli->evars.read_naddrs_max)) {
		perror("# NVM_CLI_READ_NADDRS_MAX");
		return -1;
	}

	if (_evar_noverify(cli) < 0) {
		perror("# NVM_CLI_NOVERIFY");
		return -1;
	}

	if (_evar_meta_pr(cli) < 0) {
		perror("# NVM_CLI_META_PR");
		return -1;
	}
	
	for (int i = 0; (i < cli->args.naddrs) && (!cli->evars.noverify); ++i) {
		int bounds = nvm_addr_check(cli->args.addrs[i], cli->args.dev);

		if (bounds) {
			nvm_addr_pr(cli->args.addrs[i]);
			nvm_cli_info_pr("Exceeded");
			nvm_bounds_pr(bounds);
			errno = EINVAL;
			return -1;
		}
	}

	if (cli->opts.verbose) {
		nvm_cli_info_pr("Verbose enabled -- printing CLI state -- BEGIN");
		nvm_cli_pr(cli);
		nvm_cli_info_pr("Verbose enabled -- printing CLI state -- END");
	}

	return 0;
}

int _help_wanted(int argc, char *argv[])
{
	int help_wanted = 0;

	for (int i = 0; i < argc; ++i) {
		help_wanted = strlen(argv[i]) == 2 &&
				(strncmp(argv[i], "-h", 2) == 0);
		if (help_wanted)
			break;
	}

	return help_wanted;
}

int nvm_cli_init(struct nvm_cli *cli, int argc, char *argv[])
{
	int state = 0;
	int ret;

	// Get the command-line name
	ret = _parse_cli_name(argc, argv, cli);
	if (ret < 0) {
		errno = EINVAL;
		return -1;
	}
	state += ret;

	// Set to usage state when -h or no arguments are given
	if ((argc == 1) || _help_wanted(argc, argv)) {
		cli->opts.help = 1;
		return 0;
	}

	// Get the command
	ret = _parse_cmd(argc - state, argv + state, cli);
	if (ret < 0) {
		errno = EINVAL;
		return -1;
	}
	state += ret;

	// Grab the positional command arguments
	ret = _parse_cmd_args(argc - state, argv + state, cli);
	if (ret < 0) {
		errno = EINVAL;
		return -1;
	}

	// Grab the option arguments
	ret = _parse_options(argc - state, argv + state, cli);
	if (ret < 0) {
		errno = EINVAL;
		return -1;
	}

	// Setup environment and device
	ret = _evar_and_dev_setup(cli);
	if (ret < 0) {
		if (cli->args.dev)
			nvm_dev_close(cli->args.dev);
		errno = EINVAL;
		return -1;
	}

	return 0;
}

int nvm_cli_run(struct nvm_cli *cli)
{
	int res;

	if (!cli) {
		errno = EINVAL;
		return -1;
	}

	if (cli->opts.help) {	// NOTE: -h ==> Print usage and return
		nvm_cli_usage_pr(cli);
		return 0;
	}

	res = cli->cmd.func(cli);
	if (res) {
		char msg[1024];
		sprintf(msg, "# %s", cli->cmd.name);
		perror(msg);
	}

	return res ? 1 : 0;
}

void nvm_cli_destroy(struct nvm_cli *cli)
{
	if (!cli)
		return;

	nvm_dev_close(cli->args.dev);
}

void _nvm_cli_opts_mask_pr(int mask) {
	if (!mask)
		return;

	for (int i = 0; i < 10; ++i) {
		int opt = 1 << i;

		if (!(mask & opt))
			continue;

		switch(opt) {
		case NVM_CLI_OPT_HELP:
			printf(" [-h]");
			break;
		case NVM_CLI_OPT_BRIEF:
			printf(" [-b]");
			break;
		case NVM_CLI_OPT_VERBOSE:
			printf(" [-v]");
			break;
		case NVM_CLI_OPT_STATUS:
			printf(" [-s]");
			break;
		case NVM_CLI_OPT_FILE_INPUT:
			printf(" [-i FILE]");
			break;
		case NVM_CLI_OPT_FILE_OUTPUT:
			printf(" [-o FILE]");
			break;
		case NVM_CLI_OPT_VAL_DEC:
			printf(" [-n VAL]");
			break;
		case NVM_CLI_OPT_VAL_HEX:
			printf(" [-x 0xVAL]");
			break;
		}
	}
}

void _nvm_cli_opts_mask_descr_pr(int mask) {
	if (!mask)
		return;

	for (int i = 0; i < 10; ++i) {
		int opt = 1 << i;

		if (!(mask & opt))
			continue;

		switch(opt) {
		case NVM_CLI_OPT_HELP:
			printf(" -h %5s %s", " ", "Print usage");
			break;
		case NVM_CLI_OPT_BRIEF:
			printf(" -b %5s %s", " ", "Brief, minimal output");
			break;
		case NVM_CLI_OPT_VERBOSE:
			printf(" -v %5s %s", " ", "Dump CLI state to stdout");
			break;
		case NVM_CLI_OPT_STATUS:
			printf(" -s %5s %s", " ", "Dump status msgs to stdout");
			break;
		case NVM_CLI_OPT_FILE_INPUT:
			printf(" -i %5s %s", "FILE", "Path to input file");
			break;
		case NVM_CLI_OPT_FILE_OUTPUT:
			printf(" -o %5s %s", "FILE", "Path to output file");
			break;
		case NVM_CLI_OPT_VAL_DEC:
			printf(" -n %5s %s", "val", "Integer value");
			break;
		case NVM_CLI_OPT_VAL_HEX:
			printf(" -x %5s %s", "0xVAL", "Hexadecimal value");
			break;
		case NVM_CLI_OPT_NONE:
			break;
		}
		printf("\n");
	}
}

void nvm_cli_usage_pr(struct nvm_cli *cli)
{
	int all_opts = 0;

	if (!cli)
		return;

	if (cli->title) {
		printf("%s -- ", cli->title);
		nvm_ver_pr();
		printf("\n");
	}

	if (cli->descr_short) {
		printf("\n%s\n", cli->descr_short);
	}

	printf("\nUsage:\n");

	for (int i = 0; i < cli->ncmds; ++i) {
		printf(" %s %12s ", cli->name, cli->cmds[i].name);

		switch(cli->cmds[i].arg_type) {
		case NVM_CLI_ARG_ADDR:
			printf("dev_path 0xADDR");
			break;
		case NVM_CLI_ARG_ADDR_LIST:
			printf("dev_path 0xADDR [0xADDR...]");
			break;

		case NVM_CLI_ARG_DECVAL:
			printf("dev_path val");
			break;
		case NVM_CLI_ARG_DECVAL_LIST:
			printf("dev_path val [val...]");
			break;
		case NVM_CLI_ARG_DECVAL_BEGIN_END:
			printf("dev_path begin end");
			break;

		case NVM_CLI_ARG_HEXVAL:
			printf("dev_path 0xVAL");
			break;
		case NVM_CLI_ARG_HEXVAL_LIST:
			printf("dev_path 0xVAL [0xVAL...]");
			break;

		case NVM_CLI_ARG_ADDR_LUN:
			printf("dev_path ch lun");
			break;
		case NVM_CLI_ARG_ADDR_BLK:
			printf("dev_path ch lun blk");
			break;
		case NVM_CLI_ARG_ADDR_PG:
			printf("dev_path ch lun blk pg");
			break;
		case NVM_CLI_ARG_ADDR_SEC:
			printf("dev_path ch lun pl blk pg sec");
			break;
		case NVM_CLI_ARG_ADDR_LUN_HEXVAL:
			printf("dev_path ch lun 0xVAL");
			break;
		case NVM_CLI_ARG_ADDR_BLK_HEXVAL:
			printf("dev_path ch lun blk 0xVAL");
			break;
		case NVM_CLI_ARG_ADDR_CHK_HEXVAL:
			printf("dev_path grp pu chk 0xVAL");
			break;
		case NVM_CLI_ARG_ADDR_CHK_VAL_HEXVAL:
			printf("dev_path grp pu chk NADDRS 0xOPT");
			break;

		case NVM_CLI_ARG_VCOPY:
			printf("dev_path ch lun blk ch lun blk");
			break;
		case NVM_CLI_ARG_VBLK_LINE:
			printf("dev_path ch_bgn ch_end lun_bgn lun_end blk");
			break;
		case NVM_CLI_ARG_VBLK_LINE_POS:
			printf("dev_path ch_bgn ch_end lun_bgn lun_end blk count offset");
			break;

		case NVM_CLI_ARG_COUNT_OFFSET:
			printf("dev_path count offset");
			break;

		case NVM_CLI_ARG_REGISTER:
			printf("dev_path 0xREG");
			break;
		case NVM_CLI_ARG_REGISTER_VALUE:
			printf("dev_path 0xREG 0xVAL");
			break;

		case NVM_CLI_ARG_DEV_PATH:
			printf("dev_path");
			break;

		case NVM_CLI_ARG_NONE:
			break;
		}

		all_opts |= cli->cmds[i].opt_types;
		_nvm_cli_opts_mask_pr(cli->cmds[i].opt_types);
		printf("\n");
	}

	if (all_opts) {
		printf("\nOptions:\n");
		_nvm_cli_opts_mask_descr_pr(all_opts);
	}

	if (!cli->descr_long) {
		printf("\nSee: http://lightnvm.io/liblightnvm/cli/ for usage examples\n");
		return;
	}

	if (strlen(cli->descr_long) > 4) {
		printf("\nDescription:\n");
		printf("\n%s\n", cli->descr_long);
	}

	return;
}

void nvm_cli_cmd_args_pr(struct nvm_cli_cmd_args *args) {
	printf("cli_args:\n");
	printf("  dev_path: '%s'\n", args->dev_path);

	printf("cli-args-ndec_vals(%d) {\n", args->ndec_vals);
	if (args->ndec_vals)
		for (int i = 0; i < args->ndec_vals; ++i)
			printf(" %d: %zu\n", i, args->dec_vals[i]);
	printf("},\n");

	printf("cli-args-nhex_vals(%d) {\n", args->nhex_vals);
	if (args->nhex_vals)
		for (int i = 0; i < args->nhex_vals; ++i)
			printf(" %d: 0x%016"PRIx64"\n", i, args->hex_vals[i]);
	printf("},\n");

	printf("cli-args-");
	nvm_addr_prn(args->addrs, args->naddrs);

	printf("cli-");
	nvm_dev_pr(args->dev);
}

void nvm_cli_opts_pr(struct nvm_cli_opts *opts)
{
	printf("cli_opts:\n");
	printf("  mask: 0x%08x\n", opts->mask);
	printf("  help: %d\n", opts->help);
	printf("  brief: %d\n", opts->brief);
	printf("  verbose: %d\n", opts->verbose);
	printf("  dec_val: %zu\n", opts->dec_val);
	printf("  hex_val: 0x%016"PRIx64"\n", opts->hex_val);
	printf("  file_input: %s\n", opts->file_input);
	printf("  file_output: %s\n", opts->file_output);
}

void nvm_cli_evars_pr(struct nvm_cli_evars *evars)
{
	printf("cli_evars:\n");
	printf("  be_id: %d\n", evars->be_id);
	printf("  pmode: 0x%02x\n", evars->pmode);
	printf("  meta_mode: %d\n", evars->meta_mode);
	printf("  noverify: %d\n", evars->noverify);
	printf("  erase_naddrs_max: %d\n", evars->erase_naddrs_max);
	printf("  read_naddrs_max: %d\n", evars->read_naddrs_max);
	printf("  write_naddrs_max: %d\n", evars->write_naddrs_max);
	printf("  meta_pr: %d\n", evars->meta_pr);
}

void nvm_cli_pr(struct nvm_cli *cli)
{
	printf("cli:\n");
	printf("  ncmds: %d\n", cli->ncmds);
	printf("  name: '%s'\n", cli->name);
	nvm_cli_evars_pr(&cli->evars);
	nvm_cli_opts_pr(&cli->opts);
	nvm_cli_cmd_args_pr(&cli->args);
}

void nvm_cli_info_pr(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("# ");
	vprintf(format, args);
	printf("\n");

	va_end(args);
}

void nvm_cli_perror(const char *msg)
{
	char foo[1024];

	if (strlen(msg) > 1000)
		return;

	sprintf(foo, "# %s", msg);
	perror(foo);
}

void nvm_cli_status_pr(const char *task, size_t cur, size_t total)
{
	printf("status: {task: '%s', cur: %zu, total: %zu}\n",
	       task, cur, total);
}

