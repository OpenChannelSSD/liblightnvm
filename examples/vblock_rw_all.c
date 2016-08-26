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

void ex_vblock_rw_all(const char* dev_name, const char* tgt_name)
{
	NVM_DEV dev;
	NVM_TGT tgt;
	struct geometry geo;

	NVM_VBLOCK *vblks = NULL;
	int vblks_nalloc, vblks_nres;
	int i, ret;

	char *wbuf;
	char *rbuf;

	dev = nvm_dev_open(dev_name);
	if (!dev) {
		printf("Failed opening device(%s)\n", dev_name);
		return;
	}
	geo_fill(&geo, dev);
	geo_pr(&geo);

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
	memset(wbuf, 0, geo.io_nbytes_max);
	strcpy(wbuf, "Hello world of NVM");

	tgt = nvm_tgt_open(tgt_name, 0x0);
	if (!tgt) {
		printf("Failed opening target, does it exist? Create with e.g."
		       "'nvme lnvm -d nvme0n1 -n test_target -t dflash'\n");
		return;
	}

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
	
	/* Reserve via tgt in round-robin fashion across channels and luns. */
	printf("Attempting to reserve %d\n", vblks_nalloc);
	vblks_nres = 0;
	for(i=0; i<vblks_nalloc; ++i) {
		ret = nvm_vblock_gets(vblks[vblks_nres],
				      tgt,
				      i % geo.nchannels,
				      i % geo.nluns);
		if (ret) {
			printf("F: _gets i(%d), ch(%lu), lun(%lu), tgt(%p)\n",
				i, i % geo.nchannels, i % geo.nluns, tgt);
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

	for(i=0; i<vblks_nres; ++i) {
							/* Write to media */
		int written, read;
		
		written = nvm_vblock_write(vblks[i], wbuf, 1, 0, 0x0);
		printf("written(%d)\n", written);
							
		strcpy(rbuf, "");			/* Read from media */
		read = nvm_vblock_read(vblks[i], rbuf, 1, 0, 0x0);
		printf("read(%d), rbuf(%s)\n", read, rbuf);
	}

	for(i=0; i<vblks_nres; ++i) {
		ret = nvm_vblock_put(vblks[i]);	/* Release vblock from tgt */
		if (ret) {
			printf("Failed putting block via tgt(%p)\n", tgt);
		}
	}

	for(i=0; i<vblks_nalloc; ++i) {
		nvm_vblock_free(&vblks[i]);		/* De-allocate vblock */
	}
	free(vblks);
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

	ex_vblock_rw_all(argv[1], argv[2]);

	return 0;
}
