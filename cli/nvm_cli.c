#include "nvm_cli.h"
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static size_t start, stop;

static inline size_t wclock_sample(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec + tv.tv_sec * 1000000;
}

size_t nvm_timer_start(void)
{
    start = wclock_sample();
    return start;
}

size_t nvm_timer_stop(void)
{
    stop = wclock_sample();
    return stop;
}

double nvm_timer_elapsed(void)
{
    return (stop-start)/(double)1000000.0;
}

void nvm_timer_pr(const char* tool)
{
    printf("Ran %s, elapsed wall-clock: %lf\n", tool, nvm_timer_elapsed());
}

int nvm_cli_pmode(struct nvm_dev *dev)
{
	const struct nvm_geo *geo = nvm_dev_get_geo(dev);

	char *pmode_env = getenv("NVM_CLI_PMODE");		// Check ENV
	if (getenv("NVM_CLI_PMODE")) {
		switch(atoi(pmode_env)) {
			case 4:
				if (geo->nplanes < 4) {	// Verify
					errno = EINVAL;
					return -1;
				}
				return NVM_FLAG_PMODE_QUAD;
			case 2:
				if (geo->nplanes < 2) {	// Verify
					errno = EINVAL;
					return -1;
				}
				return NVM_FLAG_PMODE_DUAL;
			case 0:
				return NVM_FLAG_PMODE_SNGL;
			default:
				errno = EINVAL;
				return -1;
		}
	}

	return nvm_dev_get_pmode(dev);
}

void nvm_cli_usage(const char *cli_name, const char *cli_description,
		   NVM_CLI_CMD cmds[], int ncmds)
{
	printf("%s -- ", cli_description);
	nvm_ver_pr();
	printf("\n\nUsage:\n");

	for (int i = 0; i < ncmds; ++i) {
		printf(" %s %10s dev_path ", cli_name, cmds[i].name);

		switch(cmds[i].argt) {
		case NVM_CLI_ARG_ADDRLIST:
			printf("addr [addr...]");
			break;
		case NVM_CLI_ARG_INTLIST:
			printf("num [num...]");
			break;
		case NVM_CLI_ARG_CH_LUN:
			printf("ch lun");
			break;
		case NVM_CLI_ARG_CH_LUN_BLK:
			printf("ch lun blk");
			break;
		case NVM_CLI_ARG_CH_LUN_BLK_PG:
			printf("ch lun blk pg");
			break;
		case NVM_CLI_ARG_CH_LUN_PL_BLK_PG_SEC:
			printf("ch lun pl blk pg sec");
			break;
		case NVM_CLI_ARG_LINE:
			printf("ch_bgn ch_end lun_bgn lun_end blk");
			break;
		case NVM_CLI_ARG_COUNT_OFFSET:
			printf("count offset");
			break;

		case NVM_CLI_ARG_NONE:
			break;
		}
		printf("\n");
	}
}

NVM_CLI_CMD *nvm_cli_setup(int argc, char **argv, NVM_CLI_CMD cmds[], int ncmds)
{
	NVM_CLI_CMD *cmd = NULL;
	char cmd_name[NVM_CLI_CMD_LEN];
	char dev_path[NVM_DEV_PATH_LEN+1];

	if (argc < 3) {		// Need at lest: <cli> <cmd> <dev_path>
		return NULL;
	}
							// Get `cmd_name`
	if (strlen(argv[1]) < 1 || strlen(argv[1]) > (NVM_CLI_CMD_LEN-1)) {
		printf("FAILED: invalid cmd(%s)\n", argv[1]);
		return NULL;
	}
	memset(cmd_name, 0, sizeof(cmd_name));
	strcpy(cmd_name, argv[1]);

	for (int i = 0; i < ncmds; ++i) {		// Get `cmd`
		if (strcmp(cmd_name, cmds[i].name) == 0) {
			cmd = &cmds[i];
			break;
		}
	}
	if (!cmd) {
		printf("FAILED: invalid cmd(%s)\n", cmd_name);
		return NULL;
	}
							// Get `dev_path`
	if (strlen(argv[2]) < 1 || strlen(argv[2]) > NVM_DEV_PATH_LEN) {
		printf("FAILED: invalid dev_path(%s)\n", argv[2]);
		return NULL;
	}
	memset(dev_path, 0, sizeof(dev_path));
	strcpy(dev_path, argv[2]);

	cmd->args.dev = nvm_dev_open(dev_path);		// Open device
	if (!cmd->args.dev) {
		perror("nvm_dev_open");
		return NULL;
	}

	if (getenv("NVM_CLI_ERASE_NADDRS_MAX")) {
		int erase_naddrs_max = atoi(getenv("NVM_CLI_ERASE_NADDRS_MAX"));
		if (nvm_dev_set_erase_naddrs_max(cmd->args.dev, erase_naddrs_max)) {
			perror("nvm_dev_erase_naddrs_max");
			return NULL;
		}
	}

	if (getenv("NVM_CLI_READ_NADDRS_MAX")) {
		int read_naddrs_max = atoi(getenv("NVM_CLI_READ_NADDRS_MAX"));
		if (nvm_dev_set_read_naddrs_max(cmd->args.dev, read_naddrs_max)) {
			perror("nvm_dev_read_naddrs_max");
			return NULL;
		}
	}

	if (getenv("NVM_CLI_WRITE_NADDRS_MAX")) {
		int write_naddrs_max = atoi(getenv("NVM_CLI_WRITE_NADDRS_MAX"));
		if (nvm_dev_set_write_naddrs_max(cmd->args.dev, write_naddrs_max)) {
			perror("nvm_dev_write_naddrs_max");
			return NULL;
		}
	}

	cmd->args.geo = nvm_dev_get_geo(cmd->args.dev);	// Get geometry
	cmd->args.addrs[0].ppa = 0;

	switch(cmd->argt) {				// Get variable params
		case NVM_CLI_ARG_CH_LUN_BLK_PG:
			if (argc < 7) {
				printf("FAILED: Invalid argc\n");
				return NULL;
			}

			cmd->args.addrs[0].g.blk = atoi(argv[6]);

		case NVM_CLI_ARG_CH_LUN_BLK:
			if (argc < 6) {
				printf("FAILED: Invalid argc\n");
				return NULL;
			}

			cmd->args.addrs[0].g.blk = atoi(argv[5]);

		case NVM_CLI_ARG_CH_LUN:
			if (argc < 5) {
				printf("FAILED: Invalid argc\n");
				return NULL;
			}

			cmd->args.addrs[0].g.lun = atoi(argv[4]);
			cmd->args.addrs[0].g.ch = atoi(argv[3]);
			cmd->args.naddrs = 1;
			break;

		case NVM_CLI_ARG_CH_LUN_PL_BLK_PG_SEC:
			if (argc != 9) {
				printf("FAILED: Invalid argc\n");
				return NULL;
			}

			cmd->args.addrs[0].g.sec = atoi(argv[8]);
			cmd->args.addrs[0].g.pg = atoi(argv[7]);
			cmd->args.addrs[0].g.blk = atoi(argv[6]);
			cmd->args.addrs[0].g.pl = atoi(argv[5]);
			cmd->args.addrs[0].g.lun = atoi(argv[4]);
			cmd->args.addrs[0].g.ch = atoi(argv[3]);
			cmd->args.naddrs = 1;
			break;

		case NVM_CLI_ARG_LINE:
			if (argc < 8) {
				printf("FAILED: Invalid argc\n");
				return NULL;
			}

			cmd->args.addrs[0].ppa = 0;	// Span begins at
			cmd->args.addrs[0].g.ch = atoi(argv[3]);
			cmd->args.addrs[0].g.lun = atoi(argv[5]);
			cmd->args.addrs[0].g.blk = atoi(argv[7]);

			cmd->args.addrs[1].ppa = 0;	// Span ends at
			cmd->args.addrs[1].g.ch = atoi(argv[4]);
			cmd->args.addrs[1].g.lun = atoi(argv[6]);
			cmd->args.addrs[1].g.blk = atoi(argv[7]);

			cmd->args.naddrs = 2;
			break;

		case NVM_CLI_ARG_ADDRLIST:
			if (argc < 4) {
				printf("FAILED: Invalid argc\n");
				return NULL;
			}

			cmd->args.naddrs = argc - 3;
			for (int i = 0; i < cmd->args.naddrs; ++i) {
				cmd->args.addrs[i].ppa = strtol(argv[i+3], NULL,
								16);
			}
			break;

		case NVM_CLI_ARG_INTLIST:
			if (argc < 4) {
				printf("FAILED: Invalid argc\n");
				return NULL;
			}

			cmd->args.nlbas = argc - 3;
			for (int i = 0; i < cmd->args.nlbas; ++i) {
				cmd->args.lbas[i] = atol(argv[i+3]);
			}
			break;


		case NVM_CLI_ARG_COUNT_OFFSET:
			cmd->args.count = atol(argv[3]);
			cmd->args.offset = atol(argv[4]);
			break;

		case NVM_CLI_ARG_NONE:
			break;
	}

	// Verify that addresses are within device bounds
	if (!getenv("NVM_CLI_NOVERIFY"))
		for (int i = 0; i < cmd->args.naddrs; ++i) {
			int bounds = nvm_addr_check(cmd->args.addrs[i],
						    cmd->args.geo);

			if (bounds) {
				nvm_addr_pr(cmd->args.addrs[i]);
				printf("Exceeded:\n");
				nvm_bounds_pr(bounds);
				return NULL;
			}
		}

	return cmd;
}

void nvm_cli_teardown(NVM_CLI_CMD *cmd)
{
	if (!cmd)
		return;

	if (cmd->args.dev) {
		nvm_dev_close(cmd->args.dev);
	}
}

