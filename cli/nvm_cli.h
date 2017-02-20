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

void nvm_cli_usage(const char *cli_name, const char *cli_description,
		   NVM_CLI_CMD cmds[], int ncmds);

NVM_CLI_CMD *nvm_cli_setup(int argc, char **argv, NVM_CLI_CMD cmds[], int ncmds);

void nvm_cli_teardown(NVM_CLI_CMD *cmd);

size_t nvm_timer_start(void);
size_t nvm_timer_stop(void);
double nvm_timer_elapsed(void);
void nvm_timer_pr(const char* tool);

/**
 * Override plane_mode via ENV("NVM_CLI_PMODE")
 *
 * @note
 * If nothing NVM_CLI_PMODE is not set, the device default is returned.
 *
 * @param dev Device to verify user-supplied plane_mode against
 * @return On success, user-supplied plane_mode is returned. On error, -1 and
 * errno set to indicate the error.
 */
int nvm_cli_pmode(struct nvm_dev *dev);

/**
 * Provide an override for device meta_mode via CLI ENV("NVM_CLI_META_MODE")
 *
 * @note
 * If nothing NVM_CLI_META_MODE is not set, the device default is returned.
 *
 * @param dev Device to verify user-supplied meta_mode against
 * @return On success, user-supplied plane_mode is returned. On error, -1 and
 * errno set to indicate the error.
 */
int nvm_cli_meta_mode(struct nvm_dev *dev);
