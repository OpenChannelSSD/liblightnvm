#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_block_pio_n(const char* dev_name)
{
	NVM_DEV dev;
	NVM_GEO geo;

	NVM_VBLOCK *vblks = NULL;
	int vblks_nalloc, vblks_nres;
	int i, ret;

	char *wbuf;
	char *rbuf;

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device, does it exist / initialized?\n");
		return;
	}

	geo = nvm_dev_attr_geo(dev);

	wbuf = nvm_vpage_buf_alloc(geo);
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
	memset(wbuf, 0, geo.vpage_nbytes);
	strcpy(wbuf, "Hello world of NVM");

	/* Allocate nblocks * nluns amount of vblocks. */
	vblks_nalloc = geo.nblocks * geo.nluns;
	vblks = malloc(sizeof(NVM_VBLOCK) * vblks_nalloc);
	for(i=0; i<vblks_nalloc; ++i) {
		vblks[i] = nvm_vblock_new();
		if (!vblks[i]) {
			printf("Failed allocating vblks[%d]\n", i);
			return;
		}
	}
	
	/* Reserve via dev in round-robin fashion across channels and luns. */
	printf("Attempting to reserve %d\n", vblks_nalloc);
	vblks_nres = 0;
	for(i=0; i<vblks_nalloc; ++i) {
		ret = nvm_vblock_gets(vblks[vblks_nres],
				      dev,
				      i % geo.nchannels,
				      i % geo.nluns);
		if (ret) {
			printf("F: _gets i(%d), ch(%lu), lun(%lu), dev(%p)\n",
				i, i % geo.nchannels, i % geo.nluns, dev);
			continue;
		}
		nvm_vblock_pr(vblks[vblks_nres]);

		vblks_nres++;
	}
	printf("Reserved %d:%luMb of %d:%luMb => %d:%luMb failed\n",
	       vblks_nres,
	       (vblks_nres * geo.vblock_nbytes) >> 20,
	       vblks_nalloc,
	       (vblks_nalloc * geo.vblock_nbytes) >> 20,
	       vblks_nalloc - vblks_nres,
	       ((vblks_nalloc - vblks_nres) * geo.vblock_nbytes) >> 20);

	for(i=0; i<vblks_nres; ++i) {	/* Write all pages then read them */
		int pg;

		for(pg=0; pg<geo.npages; ++pg) {	/* write */
			int written;

			written = nvm_vblock_pwrite(vblks[i], wbuf, 1, pg);
			if (!written)
				printf("_write failed pg(%d)\n", pg);
		}

		for(pg=0; pg<geo.npages; ++pg) {	/* read */
			int read;

			strcpy(rbuf, "");

			read = nvm_vblock_pread(vblks[i], rbuf, 1, 0);
			if (!read)
				printf("_read failed pg(%d)\n", pg);
		}
	}

	for(i=0; i<vblks_nres; ++i) {
		ret = nvm_vblock_put(vblks[i]);	/* Release vblock from dev */
		if (ret) {
			printf("Failed putting block via dev(%p)\n", dev);
		}
	}

	for(i=0; i<vblks_nalloc; ++i) {
		nvm_vblock_free(&vblks[i]);		/* De-allocate vblock */
	}
	free(vblks);
	nvm_dev_close(dev);				/* Close the device */
	free(wbuf);
	free(rbuf);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s dev_name ", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	ex_block_pio_n(argv[1]);

	return 0;
}
