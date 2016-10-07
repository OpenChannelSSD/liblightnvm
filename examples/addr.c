/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	NVM_ADDR addr;

	switch(argc) {
		case 2:
			addr.ppa = atol(argv[1]);
			break;
		case 7:
			addr.g.ch = atoi(argv[1]);
			addr.g.lun = atoi(argv[2]);
			addr.g.pl = atoi(argv[3]);
			addr.g.blk = atoi(argv[4]);
			addr.g.pg = atoi(argv[5]);
			addr.g.sec = atoi(argv[6]);
			break;
		default:
			printf("Usage: %s ppa\n", argv[0]);
			printf(" OR");
			printf("Usage: %s ch lun pl blk pg sec\n", argv[0]);
			return -1;
	}

	nvm_addr_pr(addr);

	return 0;
}
