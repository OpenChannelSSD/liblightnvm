/**
 * bbt - CLI example for bad-block-table
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	NVM_DEV dev;
	NVM_BBT* bbt;
	NVM_ADDR addr;
	NVM_RET ret = {0, 0};
	
	dev = nvm_dev_open("/dev/nvme0n1");
	if (!dev) {
		perror("nvm_dev_open");
		return 1;
	}

	addr.ppa = 0;
	addr.g.ch = 0;
	addr.g.lun = 0;

	bbt = nvm_bbt_get(dev, addr, &ret);
	if (!bbt) {
		nvm_addr_pr(addr);
		perror("nvm_bbt_get");
		nvm_ret_pr(&ret);
		nvm_dev_close(dev);
		return 1;
	}

	nvm_bbt_pr(bbt);

	free(bbt->blks);
	free(bbt);
	nvm_dev_close(dev);

	return 0;
}
