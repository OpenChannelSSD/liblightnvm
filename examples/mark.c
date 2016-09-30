/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int mark(const char *dev_name, uint16_t ch, uint16_t lun, uint16_t blk, uint16_t type)
{
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_ADDR addr;
	int err = 0;

	printf("mark{ dev_name(%s), ch(%d), lun(%d), blk(%d), type(%d) }\n",
		dev_name, ch, lun, blk, type);

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device, dev_name(%s)\n", dev_name);
		return -EINVAL;
	}
	geo = nvm_dev_attr_geo(dev);

	printf("** Device information **\n");
	nvm_dev_pr(dev);
	nvm_geo_pr(geo);

	if (ch >= geo.nchannels) {
		printf("ch(%u) too large\n", ch);
		return -EINVAL;
	}
	if (lun >= geo.nluns) {
		printf("lun(%u) too large\n", lun);
		return -EINVAL;
	}
	if (blk >= geo.nblocks) {
		printf("blk(%u) too large\n", blk);
		return -EINVAL;
	}
	switch (type) {
		case 0:
		case 1:
		case 2:
			break;
		default:
			return -EINVAL;
	}

	addr.g.ch = ch;
	addr.g.lun = lun;
	addr.g.blk = blk;

	err = nvm_dev_mark(dev, addr, type);
	if (err) {
		printf("nvm_dev_mark failed err(%d)\n", err);
	} else {
		printf("no errors detected when marking block bad.\n");
	}

	nvm_dev_close(dev);

	return err;
}

int main(int argc, char **argv)
{
	char dev_name[DISK_NAME_LEN+1];
	unsigned int ch, lun, blk, type;

	if (argc != 6) {
		printf("Usage: sudo %s dev_name ch lun blk type\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN);
	}

	memset(dev_name, 0, sizeof(dev_name));
	strcpy(dev_name, argv[1]);

	ch = atoi(argv[2]);
	lun = atoi(argv[3]);
	blk = atoi(argv[4]);
	type = atoi(argv[5]);

	return mark(dev_name, ch, lun, blk, type);
}
