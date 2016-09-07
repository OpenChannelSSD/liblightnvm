/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int gblk(const char *dev_name, uint16_t ch, uint16_t lun)
{
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_VBLOCK vblk;
	int err = 0;

	printf("gblk{ dev_name(%s), ch(%d), lun(%d) }\n", dev_name, ch, lun);

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device, dev_name(%s)\n", dev_name);
		return -EINVAL;
	}
	geo = nvm_dev_get_geo(dev);

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

	vblk = nvm_vblock_new();
	if (!vblk) {
		printf("Failed allocating vblk\n");
	} else {
		err = nvm_vblock_gets(vblk, dev, ch, lun);
		if (err) {
			printf("nvm_vblock_gets failed err(%d)\n", err);
		} else {
			printf("** Got the following vblock **\n");
			nvm_vblock_pr(vblk);
			printf("NOTE: Block was NOT released..\n");
			printf(".. that is `nvm_vblock_put` was not called.\n");
		}
	}

	nvm_dev_close(dev);

	return err;
}

int main(int argc, char **argv)
{
	char dev_name[DISK_NAME_LEN+1];
	uint16_t ch, lun;

	if (argc != 4) {
		printf("Usage: sudo %s dev_name ch lun\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN);
	}

	memset(dev_name, 0, sizeof(dev_name));
	strcpy(dev_name, argv[1]);

	ch = atoi(argv[2]);
	lun = atoi(argv[3]);

	return gblk(dev_name, ch, lun);
}
