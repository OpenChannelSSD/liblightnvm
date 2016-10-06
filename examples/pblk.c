/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int pblk(const char *dev_name, uint16_t ch, uint16_t lun, uint16_t blk)
{
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_VBLOCK vblk;
	NVM_ADDR addr;
	int err = 0;

	printf("pblk{ dev_name(%s), ch(%d), lun(%d), blk(%d) }\n",
	       dev_name, ch, lun, blk);

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device, dev_name(%s)\n", dev_name);
		return -EINVAL;
	}
	geo = nvm_dev_attr_geo(dev);

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

	addr.g.ch = ch;
	addr.g.lun = lun;
	addr.g.blk = blk;

	vblk = nvm_vblock_new_on_dev(dev, addr.ppa);
	if (!vblk) {
		printf("Failed allocating vblk\n");
	} else {
		err = nvm_vblock_put(vblk);
		if (err) {
			printf("nvm_vblock_put: failed, err(%d)\n", err);
		} else {
			printf("nvm_vblock_put: no errors detected\n");
		}
		nvm_vblock_free(&vblk);
	}

	nvm_dev_close(dev);

	return err;
}

int main(int argc, char **argv)
{
	char dev_name[DISK_NAME_LEN+1];
	uint16_t ch, lun, blk;

	if (argc != 5) {
		printf("Usage: sudo %s dev_name ch lun blk\n", argv[0]);
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

	return pblk(dev_name, ch, lun, blk);
}
