#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int write(const char *dev_name, uint16_t ch, uint16_t lun, uint16_t blk,
	 int16_t page)
{
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_VBLOCK vblock;
	NVM_ADDR addr;
	char *buf;
	int buf_len;
	ssize_t err = 0;
	uint64_t i;

	printf("write{ dev_name(%s), ch(%d), lun(%d), blk(%d), page(%d) }\n",
		dev_name, ch, lun, blk, page);

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
	if (page > 0 && page >= geo.npages) {
		printf("page(%u) too large\n", page);
		return -EINVAL;
	}

	addr.g.ch = ch;
	addr.g.lun = lun;
	addr.g.blk = blk;

	vblock = nvm_vblock_new_on_dev(dev, addr.ppa);

	if (page < 0) {
		buf = nvm_vblock_buf_alloc(geo);
		buf_len = nvm_dev_attr_vblock_nbytes(dev);
		if (!buf)
			goto done;
		for(i = 0; i < buf_len; ++i)
			buf[i] = (i % 28) + 65;
		err = nvm_vblock_write(vblock, buf);
	} else {
		buf = nvm_vpage_buf_alloc(geo);
		buf_len = nvm_dev_attr_vpage_nbytes(dev);
		if (!buf)
			goto done;
		for(i = 0; i < buf_len; ++i)
			buf[i] = (i % 28) + 65;
		err = nvm_vblock_pwrite(vblock, buf, page);
	}

	if (err) {
		printf("ERRORED: nvm_vblock_[p]write(...), err(%ld)\n", err);
	}

done:
	free(buf);
	nvm_vblock_free(&vblock);
	nvm_dev_close(dev);

	return err;
}

int main(int argc, char **argv)
{
	char dev_name[DISK_NAME_LEN+1];
	uint16_t ch, lun, blk;
	int16_t page;

	if (argc < 5) {
		printf("Usage: sudo %s dev_name ch lun blk [page]\n", argv[0]);
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
	if (argc == 6) {
		page = atoi(argv[5]);
	} else {
		page = -1;
	}

	return write(dev_name, ch, lun, blk, page);
}
