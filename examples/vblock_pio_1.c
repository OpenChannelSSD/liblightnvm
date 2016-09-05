#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_vblock_pio_1(const char* dev_name)
{
	NVM_DEV dev;
	NVM_GEO geo;
	NVM_VBLOCK vblock;
	int pg, ret, read, written;

	char *wbuf;
	char *rbuf;

	dev = nvm_dev_open(dev_name);			/* Open device */
	if (!dev) {
		printf("Failed opening device, does it exist / initialized?\n");
		return;
	}

	geo = nvm_dev_get_geo(dev);			/* Get dev geometry */

	wbuf = nvm_vpage_buf_alloc(geo);		/* Setup buffers */
	if (!wbuf) {
		printf("Failed allocating nbytes(%lu) for write buffer\n",
			geo.vpage_nbytes);
		return;
	}
	memset(wbuf, 0, geo.vpage_nbytes);
	strcpy(wbuf, "Hello World of NVM");

	rbuf = nvm_vpage_buf_alloc(geo);
	if (!rbuf) {
		printf("Failed allocating nbytes(%lu) for read buffer\n",
			geo.vpage_nbytes);
		return;

	}
	memset(rbuf, 0, geo.vpage_nbytes);
	strcpy(rbuf, "I WAS NOT WRITTEN");

	vblock = nvm_vblock_new();			/* Allocate vblock */
	if (!vblock) {
		printf("Failed allocating vblock\n");
		return;
	}

	ret = nvm_vblock_gets(vblock, dev, 0, 0);	/* Reserve vblock */
	if (ret) {
		printf("Failed getting block on dev(%p)\n", dev);
		return;
	}
	nvm_vblock_pr(vblock);

	for(pg=0; pg<geo.npages; ++pg) {		/* Write ALL pages */
		written = nvm_vblock_pwrite(vblock, wbuf, 1, pg);
		if (!written)
			printf("FAILED: page(%d) buf(%s)\n", pg, wbuf);
	}

	read = nvm_vblock_pread(vblock, rbuf, 1, 0);	/* Read a SINGLE page */
	printf("read(%d), rbuf(%s)\n", read, rbuf);

	ret = nvm_vblock_put(vblock);		/* Release vblock */
	if (ret) {
		printf("Failed putting block on dev(%p)\n", dev);
	}

	nvm_vblock_free(&vblock);		/* De-allocate vblock */
	nvm_dev_close(dev);			/* Close the device */

	free(wbuf);				/* De-allocate buffers */
	free(rbuf);
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

	ex_vblock_pio_1(argv[1]);

	return 0;
}
