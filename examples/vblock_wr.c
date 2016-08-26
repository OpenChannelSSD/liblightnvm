#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

struct geometry {
	/* Values queried from device */
	size_t nchannels;	// # of channels on device
	size_t nluns;		// # of luns per channel
	size_t nplanes;		// # of planes for lun
	size_t nblocks;		// # of blocks per plane
	size_t npages;		// # of pages per block
	size_t nsectors;	// # of sectors per page
	size_t nbytes;		// # of bytes per sector

	/* Values derived from above */
	size_t tbytes;			// Total # of bytes on device
	size_t vblock_nbytes;	// # of bytes per vblock
	size_t io_nbytes_max;	// # upper bound on _nvm_vblock_[read|write]

};

void geo_fill(struct geometry* geo, NVM_DEV dev)
{
	geo->nchannels = nvm_dev_get_nchannels(dev);
	geo->nluns = nvm_dev_get_nluns(dev);
	geo->nplanes = nvm_dev_get_nplanes(dev);
	geo->nblocks = nvm_dev_get_nblocks(dev);
	geo->npages = nvm_dev_get_npages(dev);
	geo->nsectors = nvm_dev_get_nsectors(dev);
	geo->nbytes = nvm_dev_get_nbytes(dev);

	geo->tbytes = geo->nluns * geo->nplanes * geo->nblocks * \
			geo->npages * geo->nsectors * geo->nbytes;
	geo->vblock_nbytes = geo->nplanes * geo->npages * \
				 geo->nsectors * geo->nbytes;
	geo->io_nbytes_max = geo->nplanes * geo->nsectors * geo->nbytes;
}

void geo_pr(struct geometry* geo)
{
	printf("#ch(%lu), #ln(%lu), #pl(%lu), #bl(%lu), "
		"#pg(%lu), #sc(%lu), #bt(%lu)\n",
		geo->nchannels, geo->nluns, geo->nplanes, geo->nblocks,
		geo->npages, geo->nsectors, geo->nbytes);

	printf("io_nbytes_max(%lub:%luKb)\n",
		geo->io_nbytes_max, geo->io_nbytes_max >> 10);
	printf("total_nbytes(%lub:%luMb)\n",
		geo->tbytes, geo->tbytes >> 20);
	printf("vblock_nbytes(%lub:%luMb)\n",
		geo->vblock_nbytes, geo->vblock_nbytes >> 20);
}

void ex_vblock_wr(const char* dev_name, const char* tgt_name)
{
	NVM_DEV dev;
	struct geometry geo;
	NVM_TGT tgt;
	NVM_VBLOCK vblock;
	int i, ret, r_nbytes, w_nbytes;

	char *wbuf;
	char *rbuf;

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device(%s)\n", dev_name);
		return;
	}
	geo_fill(&geo, dev);

	ret = posix_memalign((void**)&wbuf, geo.nbytes, geo.io_nbytes_max);
	if (ret) {
		printf("Failed allocating write buffer(%p)\n", wbuf);
		return;
	}
	memset(wbuf, 0, geo.io_nbytes_max);
	strcpy(wbuf, "Hello World of NVM");

	ret = posix_memalign((void**)&rbuf, geo.nbytes, geo.io_nbytes_max);
	if (ret) {
		printf("Failed allocating read buffer(%p)\n", wbuf);
		return;
	}
	memset(rbuf, 0, geo.io_nbytes_max);
	strcpy(rbuf, "I WAS NOT WRITTEN");

	tgt = nvm_tgt_open(tgt_name, 0x0);	/* Why 0x0? */
	if (!tgt) {
		printf("Failed opening target, does it exist? Create with e.g."
		       "'nvme lnvm -d nvme0n1 -n test_target -t dflash'\n");
		return;
	}

	vblock = nvm_vblock_new();		/* Allocate the vblock */
	if (!vblock) {
		printf("Failed allocating vblock\n");
		return;
	}

	ret = nvm_vblock_gets(vblock, tgt, 0, 0);	/* Reserve from tgt on lun 0 */
	if (ret) {
		printf("Failed getting block via tgt(%p)\n", tgt);
		return;
	}
	nvm_vblock_pr(vblock);

	for(i=0; i<geo.npages; i++) {
		w_nbytes = nvm_vblock_write(vblock, wbuf, 1, i, 0x0);	/* WRITE */
		printf("written(%d), wbuf(%s)\n", w_nbytes, wbuf);

		r_nbytes = nvm_vblock_read(vblock, rbuf, 1, i, 0x0);	/* READ */
		printf("read(%d), rbuf(%s)\n", r_nbytes, rbuf);
	}

	ret = nvm_vblock_put(vblock);	/* Release vblock from tgt */
	if (ret) {
		printf("Failed putting block via tgt(%p)\n", tgt);
	}

	nvm_vblock_free(&vblock);		/* De-allocate vblock */
	nvm_tgt_close(tgt);			/* Close the target */
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("Usage: %s dev_name tgt_name "
		       "e.g. '%s nvme0n1 ex_target'\n", argv[0], argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > DISK_NAME_LEN) {
		printf("len(device_name) > %d\n", DISK_NAME_LEN - 1);
	}

	ex_vblock_wr(argv[1], argv[2]);

	return 0;
}
