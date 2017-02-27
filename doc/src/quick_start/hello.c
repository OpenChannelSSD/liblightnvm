#include <stdio.h>
#include <liblightnvm.h>

int main(int argc, char **argv)
{
	struct nvm_dev *dev = nvm_dev_open("/dev/nvme0n1");
	if (!dev) {
		perror("nvm_dev_open");
		return 1;
	}
	nvm_dev_pr(dev);
	nvm_dev_close(dev);

	return 0;
}
