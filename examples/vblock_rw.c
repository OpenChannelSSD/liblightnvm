#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <liblightnvm.h>

int setup_buffers(NVM_GEO geo, char **rbuf, char **wbuf)
{

	return 0;
}

int ex_vblock_rw(const char* dev_name)
{
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_VBLOCK vblock;
	int i;

	int err = 0;
	char *wbuf = NULL;
	char *rbuf = NULL;

	dev = nvm_dev_open(dev_name);			/* Open device */
	if (!dev) {
		printf("Failed opening device, does it exist / initialized?\n");
		return -EFAULT;
	}

	geo = nvm_dev_attr_geo(dev);			/* Get dev geometry */

	rbuf = nvm_vblock_buf_alloc(geo);		/* Setup buffers */
	if (!rbuf)
		return -ENOMEM;
	memset(rbuf, 0, geo.vblock_nbytes);
	strcpy(rbuf, "I WAS NOT WRITTEN");

	wbuf = nvm_vblock_buf_alloc(geo);
	if (!wbuf) {
		free(rbuf);
		return -ENOMEM;
	}
	memset(wbuf, 0, geo.vblock_nbytes);

	for (i = 0; i < geo.vblock_nbytes-1; i++) {	/* Repeat string A-Z */
		wbuf[i] = i % 26 + 65;
	}

	vblock = nvm_vblock_new();			/* Allocate vblock */
	if (!vblock) {
		printf("Failed allocating vblock\n");
		return -ENOMEM;
	}

	err = nvm_vblock_gets(vblock, dev, 0, 0);	/* Reserve vblock */
	if (err) {
		printf("Failed getting block on dev(%p)\n", dev);
		return -EFAULT;
	}
	nvm_vblock_pr(vblock);

	err = nvm_vblock_write(vblock, wbuf);/* Write entire block */
	printf("Write: err(%d)\n", err);

	if (!err) {
		err = nvm_vblock_read(vblock, rbuf);	/* Read entire block */
		if (!err) {
			printf("%.*s\n", 26, rbuf);
			printf("%.*s\n", 26, rbuf + (geo.vblock_nbytes/2));
		}
		printf("Read: err(%d)\n", err);
	}

	err = nvm_vblock_put(vblock);		/* Release vblock */
	if (err) {
		printf("Failed putting block on dev(%p)\n", dev);
	}

	nvm_vblock_free(&vblock);		/* De-allocate vblock */
	nvm_dev_close(dev);			/* Close the device */

	free(wbuf);				/* De-allocate buffers */
	free(rbuf);

	return err;
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s dev_name\n", argv[0]);
		return -1;
	}
	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	return ex_vblock_rw(argv[1]);
}
