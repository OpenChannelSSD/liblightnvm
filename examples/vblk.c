#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <liblightnvm.h>

#define NVM_CLI_CMD_LEN 50

typedef int (*cmd_fp)(NVM_DEV, NVM_GEO, NVM_ADDR, int);

// Dump content of buf to stdout
void _dump_buf(char *buf, int buf_len)
{
	int i;

	printf("** DUMPING - BEGIN **\n");
	for(i = 0; i < buf_len; i++) {
		printf("byte_offset(%d), ", i);
		printf("val(%c)\n", buf[i]);
	}
	printf("** DUMPING - END **\n");
}

// Fill buf with pseudo-data
void _fill_buf(char **buf, int buf_len)
{
	int i;

	for(i = 0; i < buf_len; ++i)
		(*buf)[i] = (i % 28) + 65;
}

void _pr_usage(char *arg)
{
	printf("Usage(s):\n"
		" %s get dev_name ch lun\n"
		" %s put dev_name ch lun blk\n"
		" %s read dev_name ch lun blk\n"
		" %s write dev_name ch lun blk\n"
		" %s erase dev_name ch lun blk\n"
		" %s mark_b dev_name ch lun blk\n"
		" %s mark_g dev_name ch lun blk\n"
		" %s mark_h dev_name ch lun blk\n"
		" %s pread dev_name ch lun blk pg\n"
		" %s pwrite dev_name ch lun blk pg\n",
		arg, arg, arg, arg, arg, arg, arg, arg, arg, arg);
}

int get(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_VBLOCK vblk;
	ssize_t err;

	printf("** nvm_vblock_gets(..., %d, %d)\n", addr.g.ch, addr.g.lun);

	vblk = nvm_vblock_new();
	if (!vblk) {
		printf("FAILED: allocating vblk\n");
		return -ENOMEM;
	}

	err = nvm_vblock_gets(vblk, dev, addr.g.ch, addr.g.lun);
	if (err) {
		printf("FAILED: nvm_vblock_gets err(%ld)\n", err);
	} else {
		printf("got ");
		nvm_vblock_pr(vblk);
	}

	nvm_vblock_free(&vblk);

	return err;
}

int put(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_VBLOCK vblk;
	ssize_t err;

	printf("** nvm_vblock_put(...): ");
	nvm_addr_pr(addr);

	vblk = nvm_vblock_new_on_dev(dev, addr.ppa);
	if (!vblk) {
		printf("FAILED: allocating vblk\n");
		return -ENOMEM;
	}

	err = nvm_vblock_put(vblk);
	if (err) {
		printf("FAILED: nvm_vblock_put err(%ld)\n", err);
	}

	nvm_vblock_free(&vblk);

	return err;
}

int pread(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_VBLOCK vblk;
	ssize_t err;

	void *buf;
	int buf_len;

	printf("** nvm_vblock_pread(...): ");
	nvm_addr_pr(addr);

	vblk = nvm_vblock_new_on_dev(dev, addr.ppa);
	if (!vblk) {
		printf("FAILED: allocating vblk\n");
		return -ENOMEM;
	}

	buf_len = nvm_dev_attr_vpage_nbytes(dev);
	buf = nvm_buf_alloc(geo, buf_len);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		free(vblk);
		return -ENOMEM;
	}

	err = nvm_vblock_pread(vblk, buf, addr.g.pg);
	_dump_buf(buf, buf_len);
	if (err) {
		printf("FAILED: nvm_vblock_pread err(%ld)", err);
	}

	nvm_vblock_free(&vblk);
	free(buf);

	return err;
}

int read(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_VBLOCK vblk;
	ssize_t err;

	void *buf;
	int buf_len;

	printf("** nvm_vblock_read(...): ");
	nvm_addr_pr(addr);

	vblk = nvm_vblock_new_on_dev(dev, addr.ppa);
	if (!vblk) {
		printf("FAILED: allocating vblk\n");
		return -ENOMEM;
	}

	buf_len = nvm_dev_attr_vblock_nbytes(dev);
	buf = nvm_buf_alloc(geo, buf_len);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		free(vblk);
		return -ENOMEM;
	}

	err = nvm_vblock_read(vblk, buf);
	_dump_buf(buf, buf_len);
	if (err) {
		printf("FAILED: nvm_vblock_read err(%ld)", err);
	}

	nvm_vblock_free(&vblk);
	free(buf);

	return err;
}

int pwrite(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_VBLOCK vblk;
	ssize_t err;

	char *buf;
	int buf_len;

	printf("** nvm_vblock_pwrite(...): ");
	nvm_addr_pr(addr);

	vblk = nvm_vblock_new_on_dev(dev, addr.ppa);
	if (!vblk) {
		printf("FAILED: allocating vblk\n");
		return -ENOMEM;
	}

	buf_len = nvm_dev_attr_vpage_nbytes(dev);
	buf = nvm_buf_alloc(geo, buf_len);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		free(vblk);
		return -ENOMEM;
	}

	_fill_buf(&buf, buf_len);

	err = nvm_vblock_pwrite(vblk, buf, addr.g.pg);
	if (err) {
		printf("FAILED: nvm_vblock_pwrite err(%ld)\n", err);
	}

	nvm_vblock_free(&vblk);
	free(buf);

	return err;
}

int write(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_VBLOCK vblk;
	ssize_t err = 0;

	char *buf;
	int buf_len;

	printf("** nvm_vblock_write(...): ");
	nvm_addr_pr(addr);

	vblk = nvm_vblock_new_on_dev(dev, addr.ppa);
	if (!vblk) {
		printf("FAILED: allocating vblk\n");
		return -ENOMEM;
	}

	buf_len = nvm_dev_attr_vblock_nbytes(dev);
	buf = nvm_buf_alloc(geo, buf_len);
	if (!buf) {
		printf("FAILED: allocating buf\n");
		free(vblk);
		return -ENOMEM;
	}

	_fill_buf(&buf, buf_len);

	err = nvm_vblock_write(vblk, buf);
	if (err) {
		printf("FAILED: nvm_vblock_write err(%ld)\n", err);
	}

	nvm_vblock_free(&vblk);
	free(buf);

	return err;
}

int erase(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	NVM_VBLOCK vblk;
	ssize_t err = 0;

	printf("** nvm_vblock_erase(...): ");
	nvm_addr_pr(addr);

	vblk = nvm_vblock_new_on_dev(dev, addr.ppa);
	if (!vblk) {
		printf("FAILED: allocating vblk\n");
		return -ENOMEM;
	}

	err = nvm_vblock_erase(vblk);
	if (err) {
		printf("FAILED: nvm_vblock_erase err(%ld)\n", err);
	}

	nvm_vblock_free(&vblk);

	return err;
}

int mark(NVM_DEV dev, NVM_GEO geo, NVM_ADDR addr, int flags)
{
	ssize_t err;

	printf("** nvm_dev_mark(...): ");
	nvm_addr_pr(addr);

	switch(flags) {
		case 0:
		case 1:
		case 2:
			break;
		default:
			return -EINVAL;
	}

	err = nvm_dev_mark(dev, addr, flags);
	if (err) {
		printf("FAILED: nvm_dev_mark err(%ld)\n", err);
	}

	return err;
}

int main(int argc, char **argv)
{
	char cmd_name[NVM_CLI_CMD_LEN];
	char dev_name[DISK_NAME_LEN+1];
	int ret;

	cmd_fp cmd;
	int cmd_argc, cmd_flags = 0;

	NVM_DEV dev;
	NVM_GEO geo;
	NVM_ADDR addr;

	if (argc < 4) {
		_pr_usage(argv[0]);
		return -1;
	}

	if (strlen(argv[1]) > (NVM_CLI_CMD_LEN-1)) {	// Get `cmd_name`
		printf("len(cmd_name) > %d\n", (NVM_CLI_CMD_LEN-1));
	}
	memset(cmd_name, 0, sizeof(cmd_name));
	strcpy(cmd_name, argv[1]);

	if (strcmp(cmd_name, "get") == 0) {
		cmd = &get;
		cmd_argc = 5;
	} else if (strcmp(cmd_name, "put") == 0) {
		cmd = &put;
		cmd_argc = 6;
	} else if (strcmp(cmd_name, "read") == 0) {
		cmd = &read;
		cmd_argc = 6;
	} else if (strcmp(cmd_name, "write") == 0) {
		cmd = &write;
		cmd_argc = 6;
	} else if (strcmp(cmd_name, "erase") == 0) {
		cmd = &erase;
		cmd_argc = 7;
	} else if (strcmp(cmd_name, "pread") == 0) {
		cmd = &pread;
		cmd_argc = 7;
	} else if (strcmp(cmd_name, "pwrite") == 0) {
		cmd = &pwrite;
		cmd_argc = 6;
	} else if (strcmp(cmd_name, "mark_b") == 0) {
		cmd = &mark;
		cmd_argc = 6;
		cmd_flags = 0;
	} else if (strcmp(cmd_name, "mark_g") == 0) {
		cmd = &mark;
		cmd_argc = 6;
		cmd_flags = 1;
	} else if (strcmp(cmd_name, "mark_h") == 0) {
		cmd = &mark;
		cmd_argc = 6;
		cmd_flags = 2;
	} else {
		printf("Invalid cmd(%s)\n", cmd_name);
		_pr_usage(argv[0]);
		return -1;
	}

	if ((argc != cmd_argc) && (argc != 4)) {
		printf("Invalid cmd(%s) argc(%d) != %d\n",
			cmd_name, argc, cmd_argc);
		_pr_usage(argv[0]);
		return -1;
	}

	if (strlen(argv[2]) > DISK_NAME_LEN) {		// Get `dev_name`
		printf("len(dev_name) > %d\n", DISK_NAME_LEN);
		return -1;
	}
	memset(dev_name, 0, sizeof(dev_name));
	strcpy(dev_name, argv[2]);

	addr.ppa = 0;
	switch(argc) {					// Get `addr`

		case 7:					// ch lun blk pg
			addr.g.pg = atoi(argv[6]);
		case 6:					// ch lun blk
			addr.g.blk = atoi(argv[5]);
		case 5:					// ch lun
			addr.g.lun = atoi(argv[4]);
			addr.g.ch = atoi(argv[3]);
			break;

		case 4:					// ppa
			addr.ppa = atol(argv[3]);
			break;

		default:
			_pr_usage(argv[0]);
			return -1;
	}

	dev = nvm_dev_open(dev_name);			// open `dev`
	if (!dev) {
		printf("Failed opening device, dev_name(%s)\n", dev_name);
		return -EINVAL;
	}
	geo = nvm_dev_attr_geo(dev);			// Get `geo`

	if (addr.g.ch >= geo.nchannels) {		// Check `addr`
		printf("ch(%u) too large\n", addr.g.ch);
		return -EINVAL;
	}
	if (addr.g.lun >= geo.nluns) {
		printf("lun(%u) too large\n", addr.g.lun);
		return -EINVAL;
	}
	if (addr.g.blk >= geo.nblocks) {
		printf("blk(%u) too large\n", addr.g.blk);
		return -EINVAL;
	}
	if (addr.g.pg >= geo.npages) {
		printf("pg(%u) too large\n", addr.g.pg);
		return -EINVAL;
	}
	if (addr.g.sec >= geo.nsectors) {
		addr.g.sec = 0;
	}

	ret = cmd(dev, geo, addr, cmd_flags);

	nvm_dev_close(dev);				// close `dev`

	return ret;
}
