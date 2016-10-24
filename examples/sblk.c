#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <omp.h>
#include <liblightnvm.h>

int read(NVM_DEV dev, NVM_GEO geo, size_t blk_idx, int flags)
{
	return 0;
}

int write(NVM_DEV dev, NVM_GEO geo, size_t blk_idx, int flags)
{
	char *bufs[geo.nchannels];

	#pragma omp parallel for schedule(static) num_threads(geo.nchannels)
	for(int ch = 0; ch < geo.nchannels; ++ch) {
		const int buf_nbytes = geo.vpage_nbytes * geo.nluns;
		bufs[ch] = nvm_buf_alloc(geo, buf_nbytes);
		nvm_buf_fill(bufs[ch], buf_nbytes);
	}

	#pragma omp parallel for schedule(static) num_threads(geo.nchannels)
	for(int ch = 0; ch < geo.nchannels; ++ch) {
		const size_t list_len = geo.nluns * geo.nplanes * geo.nsectors;

		for (size_t pg = 0; pg < geo.npages; ++pg) {

			NVM_ADDR list[list_len];

			for (size_t idx = 0; idx < list_len; ++idx) {
				uint64_t sec = idx % geo.nsectors;
				uint64_t pl = (idx / geo.nsectors) % geo.nplanes;
				uint64_t lun = ((idx / geo.nsectors) / geo.nplanes) % geo.nluns;

				list[idx].g.sec = sec;
				list[idx].g.pg = pg;
				list[idx].g.blk = blk_idx;
				list[idx].g.pl = pl;
				list[idx].g.lun = lun;
				list[idx].g.ch = ch;
			}

			ssize_t err = nvm_addr_write(dev, list, list_len, bufs[ch]);
			if (err) {
				printf("err(%ld)\n", err);
			}
		}
	}

	#pragma omp parallel for schedule(static) num_threads(geo.nchannels)
	for(int ch = 0; ch < geo.nchannels; ++ch) {
		free(bufs[ch]);
	}

	return 0;
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
	int (*func)(NVM_DEV, NVM_GEO, size_t blk_idx, int);
	int argc;
	int flags;
} NVM_CLI_VBLK_CMD;

static NVM_CLI_VBLK_CMD cmds[] = {
	{"read", read, 4, 0},
	{"write", write, 4, 0},
};

static int ncmds = sizeof(cmds) / sizeof(cmds[0]);
static char *args[] = {"dev_name", "blk"};

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
	size_t blk_idx;

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

	blk_idx = atol(argv[3]);

	dev = nvm_dev_open(dev_name);			// open `dev`
	if (!dev) {
		printf("Failed opening device, dev_name(%s)\n", dev_name);
		return -EINVAL;
	}
	geo = nvm_dev_attr_geo(dev);			// Get `geo`

	ret = cmd->func(dev, geo, blk_idx, cmd->flags);

	nvm_dev_close(dev);				// close `dev`

	return ret;
}
