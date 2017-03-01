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
 * @file liblightnvm.h
 */
#ifndef __LIBLIGHTNVM_CLI_H
#define __LIBLIGHTNVM_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <liblightnvm.h>

#define NVM_CLI_CMD_LEN 50

typedef enum nvm_cli_argtype {
	NVM_CLI_ARG_NONE,
	NVM_CLI_ARG_ADDRLIST,
	NVM_CLI_ARG_INTLIST,
	NVM_CLI_ARG_CH_LUN,
	NVM_CLI_ARG_CH_LUN_BLK,
	NVM_CLI_ARG_CH_LUN_BLK_PG,
	NVM_CLI_ARG_CH_LUN_PL_BLK_PG_SEC,
	NVM_CLI_ARG_LINE,
	NVM_CLI_ARG_COUNT_OFFSET,
} NVM_CLI_CMD_ARGTYPE;

typedef struct {
	struct nvm_dev *dev;
	const struct nvm_geo *geo;
	struct nvm_addr addrs[1024];
	int naddrs;
	size_t lbas[1024];
	int nlbas;
	size_t count;
	ssize_t offset;
} NVM_CLI_CMD_ARGS;

typedef int (*NVM_CLI_CMD_FUNC)(NVM_CLI_CMD_ARGS*);

typedef struct {
	char name[NVM_CLI_CMD_LEN];
	NVM_CLI_CMD_FUNC func;
	NVM_CLI_CMD_ARGTYPE argt;
	NVM_CLI_CMD_ARGS *args;
} NVM_CLI_CMD;

/**
 * Prints usage
 *
 * @param cli_name Name of the CLI tool
 * @param cli_description Description of the CLI tool
 * @param cmds Array of commands in CLI tool
 * @param ncmds Count of commands in CLI tool
 */
void nvm_cli_usage(const char *cli_name, const char *cli_description,
		   NVM_CLI_CMD cmds[], int ncmds);

/**
 * Parse command-line arguments and setup CLI cmd
 *
 * @param argc argument count, forward from main(int argc,...)
 * @param argv arguments, forward from main(... , char **argv)
 * @param cmds Array of commands in CLI tool
 * @param ncmds Count of commands in CLI tool
 * @return Pointer to CLI command to execute
 */
NVM_CLI_CMD *nvm_cli_setup(int argc, char **argv, NVM_CLI_CMD cmds[], int ncmds);

/**
 * Tear down CLI arguments for the given cmd
 *
 * @param cmd
 */
void nvm_cli_teardown(NVM_CLI_CMD *cmd);

/**
 * Start the global timer
 *
 * @return timestamp, in ms, when the timer was started
 */
size_t nvm_timer_start(void);

/**
 * Stop the global timer
 *
 * @return timestamp, in ms, when the timer was stopped
 */
size_t nvm_timer_stop(void);

/**
 * Return elapsed time
 *
 * @return Elapsed time, in seconds
 */
double nvm_timer_elapsed(void);

/**
 * Print out elapsed time prefix with the given string
 *
 * @param tool Prefix to use
 */
void nvm_timer_pr(const char* tool);

/**
 * Provide backend via ENV("NVM_CLI_BE_ID")
 *
 * @note
 * If NVM_CLI_BE_ID is not set, then NVM_BE_ANY is returned
 */
int nvm_cli_be_id(void);

/**
 * Override plane_mode via ENV("NVM_CLI_PMODE")
 *
 * @note
 * If NVM_CLI_PMODE is not set, the device default is returned.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, user-supplied plane_mode is returned. On error, -1 and
 * errno set to indicate the error.
 */
int nvm_cli_pmode(struct nvm_dev *dev);

/**
 * Provide an override for device meta_mode via CLI ENV("NVM_CLI_META_MODE")
 *
 * @note
 * If NVM_CLI_META_MODE is not set, the device default is returned.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, user-supplied plane_mode is returned. On error, -1 and
 * errno set to indicate the error.
 */
int nvm_cli_meta_mode(struct nvm_dev *dev);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM_CLI.H */
