/* Target info example */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

int addr_read(const char *dev_name, NVM_ADDR list[], int len)
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

	buf_len = len * geo.nbytes;
	buf = malloc(sizeof(char) * buf_len);
	if (!buf) {
		printf("Failed allocating buf\n");
		goto done;
	}

	printf("Reading the following addresses from dev_name(%s)\n", dev_name);
	for (i = 0; i < len; ++i) {
		nvm_addr_pr(list[i]);
	}

	err = nvm_addr_read(dev, list, len, buf);

	printf("** DUMPING - BEGIN **\n");
	for(i = 0; i < buf_len; ++i) {
		printf("%lu:%c\n", i, ((char*)buf)[i]);
	}
	printf("** DUMPING - END **\n");

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
	NVM_ADDR list[256];
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

	// Parse comma-separated list of addresses
	len = argc - 2;
	for (i = 0; i < len; ++i) {
		list[i].ppa = atoi(argv[i+2]);
	}

	return addr_read(dev_name, list, len);
}
