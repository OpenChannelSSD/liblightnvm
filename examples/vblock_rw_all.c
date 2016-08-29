#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <liblightnvm.h>

void ex_vblock_rw_all(const char* dev_name, const char* tgt_name)
{
	NVM_TGT tgt;
	NVM_GEO geo;

	NVM_VBLOCK *vblks = NULL;
	int vblks_nalloc, vblks_nres;
	int i, ret;

	char *wbuf;
	char *rbuf;

	tgt = nvm_tgt_open(tgt_name, 0x0);
	if (!tgt) {
		printf("Failed opening target, does it exist? Create with e.g."
		       "'nvme lnvm -d nvme0n1 -n test_target -t dflash'\n");
		return;
	}

	geo = nvm_dev_get_geo(nvm_tgt_get_dev(tgt));

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
		int page_offset;
		for(page_offset=0; page_offset<geo.npages; ++page_offset) {
			int written;
			written = nvm_vblock_write(vblks[i], wbuf, 1, page_offset);
			if (!written)
				printf("_write failed page_offset(%d)\n", page_offset);
		}
		for(page_offset=0; page_offset<geo.npages; ++page_offset) {
			int read;
			strcpy(rbuf, "");			/* Read from media */
			read = nvm_vblock_read(vblks[i], rbuf, 1, 0);
			if (!read)
				printf("_read failed page_offset(%d)\n", page_offset);
		}
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
