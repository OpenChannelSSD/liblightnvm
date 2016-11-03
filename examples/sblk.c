#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

#include <sys/time.h>

size_t start, stop;

size_t wclock_sample(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec + tv.tv_sec * 1000000;
}

size_t timer_start(void)
{
    start = wclock_sample();
    return start;
}

size_t timer_stop(void)
{
    stop = wclock_sample();
    return stop;
}

double timer_elapsed(void)
{
    return (stop-start)/(double)1000000.0;
}

void timer_pr(const char* tool)
{
    printf("Ran %s, elapsed wall-clock: %lf\n", tool, timer_elapsed());
}

int erase(NVM_DEV dev, NVM_GEO geo, NVM_SBLK sblk, int flags)
{
	ssize_t err;

	NVM_GEO sblk_geo = nvm_sblk_attr_geo(sblk);

	printf("** nvm_sblk_erase(...): sblk_tbytes(%lu)\n", sblk_geo.tbytes);
	nvm_sblk_pr(sblk);

	timer_start();
	err = nvm_sblk_erase(sblk);
	if (err) {
		printf("FAILED: nvm_sblk_erase err(%ld)\n", err);
	}
	timer_stop();
	timer_pr("nvm_sblk_erase");

	return err;
}

int write(NVM_DEV dev, NVM_GEO geo, NVM_SBLK sblk, int flags)
{
	ssize_t err;
	char *buf;

	NVM_GEO sblk_geo = nvm_sblk_attr_geo(sblk);

	printf("** nvm_sblk_write(...): sblk_tbytes(%lu)\n", sblk_geo.tbytes);
	nvm_sblk_pr(sblk);
	
	timer_start();
	buf = nvm_buf_alloc(geo, sblk_geo.tbytes);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		return -ENOMEM;
	}
	timer_stop();
	timer_pr("nvm_buf_alloc");

	timer_start();
	nvm_buf_fill(buf, sblk_geo.tbytes);
	timer_stop();
	timer_pr("nvm_buf_fill");

	timer_start();
	err = nvm_sblk_write(sblk, buf, geo.npages);
	if (err) {
		printf("FAILED: nvm_sblk_write err(%ld)\n", err);
	}
	timer_stop();
	timer_pr("nvm_sblk_write");

	free(buf);

	return err;
}

int read(NVM_DEV dev, NVM_GEO geo, NVM_SBLK sblk, int flags)
{
	ssize_t err;
	char *buf;

	NVM_GEO sblk_geo = nvm_sblk_attr_geo(sblk);

	printf("** nvm_sblk_read(...): sblk_tbytes(%lu)\n", sblk_geo.tbytes);
	nvm_sblk_pr(sblk);

	timer_start();
	buf = nvm_buf_alloc(geo, sblk_geo.tbytes);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		nvm_sblk_free(sblk);
		return -ENOMEM;
	}
	timer_stop();
	timer_pr("nvm_buf_alloc");

	timer_start();
	err = nvm_sblk_read(sblk, buf, geo.npages);
	if (err) {
		printf("FAILED: nvm_sblk_read err(%ld)\n", err);
	}
	timer_stop();
	timer_pr("nvm_sblk_read");

	free(buf);

	return err;
}

// From hereon out the code is mostly boiler-plate for command-line parsing,
// there is a bit of useful code exemplifying:
//
//  * nvm_dev_open
//  * nvm_dev_close
//  * nvm_dev_attr_geo
//
// as well as using the NVM_ADDR data structure.

#define NVM_CLI_CMD_LEN 50

typedef struct {
	char name[NVM_CLI_CMD_LEN];
	int (*func)(NVM_DEV, NVM_GEO, NVM_SBLK, int);
	int argc;
	int flags;
} NVM_CLI_VBLK_CMD;

static NVM_CLI_VBLK_CMD cmds[] = {
	{"erase", erase, 8, 0x0},
	{"write", write, 8, 0x0},
	{"read", read, 8, 0x0},
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);
static char *args[] = {
	"dev_name",
	"ch_bgn",
	"ch_end",
	"lun_bgn",
	"lun_end",
	"blk"
};

void _usage_pr(char *cli_name)
{
	int cmd;

	printf("Usage:\n");
	for (cmd = 0; cmd < ncmds; cmd++) {
		int arg;
		printf(" %s %6s", cli_name, cmds[cmd].name);
		for (arg = 0; arg < cmds[cmd].argc-2; ++arg) {
			printf(" %s", args[arg]);
		}
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	char cmd_name[NVM_CLI_CMD_LEN];
	char dev_name[DISK_NAME_LEN+1];
	int ret, i;

	NVM_CLI_VBLK_CMD *cmd = NULL;
	
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_SBLK sblk;
	int ch_bgn, ch_end, lun_bgn, lun_end, blk;

	if (argc < 3) {
		_usage_pr(argv[0]);
		return -1;
	}
							// Get `cmd_name`
	if (strlen(argv[1]) < 1 || strlen(argv[1]) > (NVM_CLI_CMD_LEN-1)) {
		printf("Invalid cmd\n");
		_usage_pr(argv[0]);
		return -EINVAL;
	}
	memset(cmd_name, 0, sizeof(cmd_name));
	strcpy(cmd_name, argv[1]);

	for (i = 0; i < ncmds; ++i) {			// Get `cmd`
		if (strcmp(cmd_name, cmds[i].name) == 0) {
			cmd = &cmds[i];
			break;
		}
	}
	if (!cmd) {
		printf("Invalid cmd(%s)\n", cmd_name);
		_usage_pr(argv[0]);
		return -EINVAL;
	}

	if (argc != cmd->argc) {			// Check argument count
		printf("Invalid cmd(%s) argc(%d) != %d\n",
			cmd_name, argc, cmd->argc);
		_usage_pr(argv[0]);
		return -1;
	}

	if (strlen(argv[2]) > DISK_NAME_LEN) {		// Get `dev_name`
		printf("len(dev_name) > %d\n", DISK_NAME_LEN);
		return -1;
	}
	memset(dev_name, 0, sizeof(dev_name));
	strcpy(dev_name, argv[2]);

	ch_bgn = atol(argv[3]);
	ch_end = atol(argv[4]);

	lun_bgn = atol(argv[5]);
	lun_end = atol(argv[6]);

	blk = atol(argv[7]);

	dev = nvm_dev_open(dev_name);			// open `dev`
	if (!dev) {
		printf("Failed opening device, dev_name(%s)\n", dev_name);
		return -EINVAL;
	}
	geo = nvm_dev_attr_geo(dev);			// Get `geo`

	sblk = nvm_sblk_new(dev, ch_bgn, ch_end, lun_bgn, lun_end, blk);
	if (!sblk) {
		printf("FAILED: allocating sblk\n");
		return -ENOMEM;
	}

	ret = cmd->func(dev, geo, sblk, cmd->flags);

	nvm_sblk_free(sblk);
	nvm_dev_close(dev);				// close `dev`

	return ret;
}
