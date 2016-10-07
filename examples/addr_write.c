/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int addr_write(const char *dev_name, NVM_ADDR list[], int len)
{
	NVM_DEV dev;
	NVM_GEO geo;
	void *buf;
	int buf_len;
	uint64_t i;
	ssize_t err = 0;

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device, dev_name(%s)\n", dev_name);
		return -EINVAL;
	}
	geo = nvm_dev_attr_geo(dev);

	buf_len = len * geo.nbytes;		// Construct buffer
	buf = nvm_buf_alloc(geo, buf_len);
	if (!buf) {
		printf("Failed allocating buf\n");
		goto done;
	}
	for(i = 0; i < buf_len; ++i)
		((char*)buf)[i] = (i % 28) + 65;

	printf("Writing the following addresses to dev_name(%s)\n", dev_name);
	for (i = 0; i < len; ++i) {
		nvm_addr_pr(list[i]);
	}

	err = nvm_addr_write(dev, list, len, buf);
	if (err) {
		printf("Detected err(%ld)\n", err);
	}

done:
	free(buf);
	nvm_dev_close(dev);

	return err;
}

int main(int argc, char **argv)
{
	char dev_name[DISK_NAME_LEN+1];
	NVM_ADDR list[1024];
	int i, len;

	if (argc < 3) {
		printf("Usage: sudo %s dev_name ppa ppa ... ppa\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN);
	}

	memset(dev_name, 0, sizeof(dev_name));
	strcpy(dev_name, argv[1]);

	len = argc - 2;			// Grab addresses
	for (i = 0; i < len; ++i) {
		list[i].ppa = atol(argv[i+2]);
	}

	return addr_write(dev_name, list, len);
}
