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
/**
 * @file liblightnvm_cli.h
 */
#ifndef __LIBLIGHTNVM_CLI_H
#define __LIBLIGHTNVM_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <liblightnvm.h>

#define NVM_CLI_CMD_LEN 50

/**
 * Enumeration of environment variables
 */
enum nvm_cli_evar_type {
	NVM_CLI_ENV_BE_ID,
	NVM_CLI_PMODE,
	NVM_CLI_META_MODE,
	NVM_CLI_NOVERIFY,
	NVM_CLI_ERASE_NADDRS_MAX,
	NVM_CLI_READ_NADDRS_MAX,
	NVM_CLI_WRITE_NADDRS_MAX,
};

/**
 * Storage for parsed environment variable dec_vals
 */
struct nvm_cli_evars {
	int be_id;
	int pmode;
	int meta_mode;
	int noverify;
	int erase_naddrs_max;
	int read_naddrs_max;
	int write_naddrs_max;
	int meta_pr;
};

/**
 * Positional argument types
 */
enum nvm_cli_cmd_arg_type {
	NVM_CLI_ARG_NONE,
	NVM_CLI_ARG_DEV_PATH,
	NVM_CLI_ARG_DECVAL,
	NVM_CLI_ARG_DECVAL_LIST,
	NVM_CLI_ARG_DECVAL_BEGIN_END,
	NVM_CLI_ARG_HEXVAL,
	NVM_CLI_ARG_HEXVAL_LIST,
	NVM_CLI_ARG_ADDR,
	NVM_CLI_ARG_ADDR_LIST,
	NVM_CLI_ARG_ADDR_LUN,
	NVM_CLI_ARG_ADDR_BLK,
	NVM_CLI_ARG_ADDR_PG,
	NVM_CLI_ARG_ADDR_SEC,
	NVM_CLI_ARG_ADDR_LUN_HEXVAL,
	NVM_CLI_ARG_ADDR_BLK_HEXVAL,
	NVM_CLI_ARG_ADDR_CHK_HEXVAL,
	NVM_CLI_ARG_ADDR_CHK_VAL_HEXVAL,
	NVM_CLI_ARG_VCOPY,
	NVM_CLI_ARG_VBLK_LINE,
	NVM_CLI_ARG_VBLK_LINE_POS,
	NVM_CLI_ARG_REGISTER,
	NVM_CLI_ARG_REGISTER_VALUE,
	NVM_CLI_ARG_COUNT_OFFSET,
};

/**
 * Storage for positional arguments
 */
struct nvm_cli_cmd_args {
	char dev_path[1024];		///< For ALL
	struct nvm_dev *dev;		///< For ALL
	const struct nvm_geo *geo;	///< For ALL
	struct nvm_addr addrs[1024];	///< Parsed gen addresses
	int naddrs;			///< Number of parsed gen addresses
	size_t dec_vals[1024];		///< Parsed decimal dec_vals
	int ndec_vals;			///< Number of parsed decical values
	uint64_t hex_vals[1024];	///< Parsed decimal values
	int nhex_vals;			///< Number of parsed decical values
};

/**
 * Types of option-arguments
 */
enum nvm_cli_opt_type {
	NVM_CLI_OPT_NONE = 0,
	NVM_CLI_OPT_HELP = 1,
	NVM_CLI_OPT_BRIEF = 1 << 1,
	NVM_CLI_OPT_VERBOSE = 1 << 2,
	NVM_CLI_OPT_STATUS = 1 << 3,
	NVM_CLI_OPT_FILE_INPUT = 1 << 4,
	NVM_CLI_OPT_FILE_OUTPUT = 1 << 5,
	NVM_CLI_OPT_VAL_DEC = 1 << 6,
	NVM_CLI_OPT_VAL_HEX = 1 << 7,
};

#define NVM_CLI_OPT_DEFAULT (NVM_CLI_OPT_HELP | NVM_CLI_OPT_VERBOSE)

/**
 * Storage for opt-arguments
 */
struct nvm_cli_opts {
	int mask;		///< Mask of all provided options
	int help;		///< For NVM_CLI_OPT_HELP
	int brief;		///< For NVM_CLI_OPT_BRIEF
	int verbose;		///< For NVM_CLI_OPT_VERBOSE
	int status;		///< FOR NVM_CLI_OPT_STATUS
	char *file_input;	///< For NVM_CLI_OPT_FILE_INPUT
	char *file_output;	///< For NVM_CLI_OPT_FILE_OUTPUT
	size_t dec_val;		///< For NVM_CLI_OPT_VAL_DEC
	size_t hex_val;		///< For NVM_CLI_OPT_VAL_HEX
};

struct nvm_cli;

typedef int (*nvm_cli_cmd_func)(struct nvm_cli*);

struct nvm_cli_cmd {
	char name[NVM_CLI_CMD_LEN];	// Command name
	nvm_cli_cmd_func func;		// Command function
	enum nvm_cli_cmd_arg_type arg_type;	// Positional argument type
	int opt_types;			// Mask of supported options
};

struct nvm_cli {
	const char *title;		// Defined by user
	const char *descr_short;	// Defined by user
	const char *descr_long;	// Defined by user

	struct nvm_cli_cmd *cmds;	// Defined by user
	int ncmds;			// Defined by user

	char name[NVM_CLI_CMD_LEN];	// Constructed by _parse_cli

	struct nvm_cli_cmd cmd;		// Selected from `cmds` by _parse_cmd

	struct nvm_cli_cmd_args args;	// Constructed by _parse_args
	struct nvm_cli_opts opts;	// Constructed by _parse_opts
	struct nvm_cli_evars evars;	// Constructed from ENV
};

void nvm_cli_cmd_args_pr(struct nvm_cli_cmd_args *args);

void nvm_cli_opts_pr(struct nvm_cli_opts *opts);

void nvm_cli_evars_pr(struct nvm_cli_evars *evars);

void nvm_cli_pr(struct nvm_cli *cli);

void nvm_cli_info_pr(const char *format, ...);

void nvm_cli_perror(const char *msg);

void nvm_cli_status_pr(const char *task, size_t cur, size_t total);

/**
 * Start the global timer
 *
 * @return timestamp, in ms, when the timer was started
 */
size_t nvm_cli_timer_start(void);

/**
 * Stop the global timer
 *
 * @return timestamp, in ms, when the timer was stopped
 */
size_t nvm_cli_timer_stop(void);

/**
 * Return elapsed time in seconds
 *
 * @return Elapsed time, in seconds
 */
double nvm_cli_timer_elapsed(void);

/**
 * Return elapsed time in seconds
 *
 * @return Elapsed time, in seconds
 */
double nvm_cli_timer_elapsed_secs(void);

/**
 * Return elapsed time in miliseconds
 *
 * @return Elapsed time, in miliseconds
 */
double nvm_cli_timer_elapsed_msecs(void);

/**
 * Return elapsed time in microseconds
 *
 * @return Elapsed time, in microseconds
 */
double nvm_cli_timer_elapsed_usecs(void);

/**
 * Return elapsed time in nanoseconds
 *
 * @return Elapsed time, in nanoseconds
 */
size_t nvm_cli_timer_elapsed_nsecs(void);

/**
 * Print out elapsed time prefix with the given string
 *
 * @param tool Prefix to use
 */
void nvm_cli_timer_pr(const char *tool);

void nvm_cli_timer_bw_pr(const char *prefix, size_t nbytes);

void nvm_cli_usage_pr(struct nvm_cli *cli);

int nvm_cli_init(struct nvm_cli *cli, int argc, char *argv[]);

int nvm_cli_run(struct nvm_cli *cli);

void nvm_cli_destroy(struct nvm_cli *cli);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM_CLI.H */
