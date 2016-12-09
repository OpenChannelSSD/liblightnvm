#include <liblightnvm.h>

#define NVM_CLI_CMD_LEN 50

typedef enum nvm_cli_argtype {
	NVM_CLI_ARG_NONE,
	NVM_CLI_ARG_PPALIST,
	NVM_CLI_ARG_LBALIST,
	NVM_CLI_ARG_CH_LUN,
	NVM_CLI_ARG_CH_LUN_BLK,
	NVM_CLI_ARG_CH_LUN_BLK_PG,
	NVM_CLI_ARG_CH_LUN_PL_BLK_PG_SEC,
	NVM_CLI_ARG_SBLK,
  NVM_CLI_ARG_COUNT_OFFSET,
} NVM_CLI_CMD_ARGTYPE;

typedef struct {
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_SBLK sblk;
	NVM_GEO sblk_geo;
	NVM_ADDR addrs[1024];
	int naddrs;
	size_t lbas[1024];
	int nlbas;
	size_t count;
	ssize_t offset;
} NVM_CLI_CMD_ARGS;

typedef int (*NVM_CLI_CMD_FUNC)(NVM_CLI_CMD_ARGS*, int);

typedef struct {
	char name[NVM_CLI_CMD_LEN];
	NVM_CLI_CMD_FUNC func;
	NVM_CLI_CMD_ARGTYPE argt;
	int flags;
	NVM_CLI_CMD_ARGS args;
} NVM_CLI_CMD;

void nvm_cli_usage(const char *cli_name, const char *cli_description,
		   NVM_CLI_CMD cmds[], int ncmds);

NVM_CLI_CMD *nvm_cli_setup(int argc, char **argv, NVM_CLI_CMD cmds[], int ncmds);

void nvm_cli_teardown(NVM_CLI_CMD *cmd);

size_t nvm_timer_start(void);
size_t nvm_timer_stop(void);
double nvm_timer_elapsed(void);
void nvm_timer_pr(const char* tool);
