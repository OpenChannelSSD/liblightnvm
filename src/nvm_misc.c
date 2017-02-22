#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <linux/lightnvm.h>
#include <linux/nvme_ioctl.h>
#include <liblightnvm.h>
#include <nvm_debug.h>
#include <nvm_dev.h>


#define CHUNK_OFFSET	4		/* BEWARE, this MUST align with FW */
//#define BIT2MASK(bitcount) ((1 << (bitcount)) - 1)
#define BITS_PER_LONG 64
#define BIT2MASK(nr)		((1UL << ((nr) % BITS_PER_LONG)) - 1)

inline uint64_t nvm_addr_host2nand(struct nvm_dev *dev, uint64_t hostppa)
{
	uint64_t d_addr = 0;
	uint16_t ch, sec, pl, lun, pg, blk;
	uint8_t ch_offt = 0;
	uint8_t sec_offt = ch_offt + dev->ppaf.n.ch_len;
	uint8_t pl_offt = sec_offt + dev->ppaf.n.sec_len;
	uint8_t lun_offt = pl_offt + dev->ppaf.n.pl_len;
	uint8_t pg_offt = lun_offt + dev->ppaf.n.lun_len;
	uint8_t blk_offt = pg_offt + dev->ppaf.n.pg_len;

	ch  = (hostppa >> dev->ppaf.n.ch_off)  & BIT2MASK(dev->ppaf.n.ch_len);
	sec = (hostppa >> dev->ppaf.n.sec_off) & BIT2MASK(dev->ppaf.n.sec_len);
	pl  = (hostppa >> dev->ppaf.n.pl_off) & BIT2MASK(dev->ppaf.n.pl_len);
	lun = (hostppa >> dev->ppaf.n.lun_off) & BIT2MASK(dev->ppaf.n.lun_len);
	pg  = (hostppa >> dev->ppaf.n.pg_off) & BIT2MASK(dev->ppaf.n.pg_len);
	blk = (hostppa >> dev->ppaf.n.blk_off) & BIT2MASK(dev->ppaf.n.blk_len);

	/*
	ch  = (hostppa >> dev->ppaf.n.ch_off)  & (hostppa & dev->mask.n.ch);
	sec = (hostppa >> dev->ppaf.n.sec_off) & (hostppa & dev->mask.n.sec);
	pl  = (hostppa >> dev->ppaf.n.pl_off) & (hostppa & dev->mask.n.pl);
	lun = (hostppa >> dev->ppaf.n.lun_off) & (hostppa & dev->mask.n.lun);
	pg  = (hostppa >> dev->ppaf.n.pg_off) & (hostppa & dev->mask.n.pg);
	blk = (hostppa >> dev->ppaf.n.blk_off) & (hostppa & dev->mask.n.blk);
	*/
	
	blk += CHUNK_OFFSET;

	d_addr |= ((uint64_t)ch) << ch_offt;
	d_addr |= ((uint64_t)lun) << lun_offt;
	d_addr |= ((uint64_t)pl) << pl_offt;
	d_addr |= ((uint64_t)blk) << blk_offt;
	d_addr |= ((uint64_t)pg) << pg_offt;
	d_addr |= ((uint64_t)sec) << sec_offt;

	return d_addr;
}

inline uint64_t nvm_addr_nand2host(struct nvm_dev *dev, uint64_t nandppa)
{
	uint64_t d_addr = 0;
	uint16_t ch, sec, pl, lun, pg, blk;

	// CNEX PPAF is fixed, so this offset is alway follow below
	uint8_t ch_offt = 0;
	uint8_t sec_offt = ch_offt + dev->ppaf.n.ch_len;
	uint8_t pl_offt = sec_offt + dev->ppaf.n.sec_len;
	uint8_t lun_offt = pl_offt + dev->ppaf.n.pl_len;
	uint8_t pg_offt = lun_offt + dev->ppaf.n.lun_len;
	uint8_t blk_offt = pg_offt + dev->ppaf.n.pg_len;

	ch  = (nandppa >> ch_offt)  & BIT2MASK(dev->ppaf.n.ch_len);
	sec = (nandppa >> sec_offt) & BIT2MASK(dev->ppaf.n.sec_len);
	pl  = (nandppa >> pl_offt) & BIT2MASK(dev->ppaf.n.pl_len);
	lun = (nandppa >> lun_offt) & BIT2MASK(dev->ppaf.n.lun_len);
	pg  = (nandppa >> pg_offt) & BIT2MASK(dev->ppaf.n.pg_len);
	blk = (nandppa >> blk_offt) & BIT2MASK(dev->ppaf.n.blk_len);

	/*
	ch  = (nandppa >> ch_offt)  & (nandppa & dev->mask.n.ch);
	sec = (nandppa >> sec_offt) & (nandppa & dev->mask.n.sec);
	pl  = (nandppa >> pl_offt) & (nandppa & dev->mask.n.pl);
	lun = (nandppa >> lun_offt) & (nandppa & dev->mask.n.lun);
	pg  = (nandppa >> pg_offt) & (nandppa & dev->mask.n.pg);
	blk = (nandppa >> blk_offt) & (nandppa & dev->mask.n.blk);
	*/
	
	if (blk < CHUNK_OFFSET) {
		printf("blk:%d is Reserved for metadata hidden by FW\n", blk);
		return BIT2MASK(64);
	} else {
		// valid visible chunk for Host
		blk -= CHUNK_OFFSET;
	}

	d_addr |= ((uint64_t)ch) << dev->ppaf.n.ch_off;
	d_addr |= ((uint64_t)lun) << dev->ppaf.n.lun_off;
	d_addr |= ((uint64_t)pl) << dev->ppaf.n.pl_off;
	d_addr |= ((uint64_t)blk) << dev->ppaf.n.blk_off;
	d_addr |= ((uint64_t)pg) << dev->ppaf.n.pg_off;
	d_addr |= ((uint64_t)sec) << dev->ppaf.n.sec_off;

	return d_addr;
}


uint32_t nvme_register_read(struct nvm_dev *dev, uint32_t regaddr)
{
	int err;
	struct nvme_misc_cmd ctl;

	memset(&ctl, 0, sizeof(ctl));
	ctl.cmdtype = NVME_MISC_RD_REGISTER;
	ctl.regaddr = regaddr;
	
	err = ioctl(dev->fd, NVME_IOCTL_MISCTOOL, &ctl);
	if (err)
		printf("rdreg retrun invalid");

	return ctl.val32;
}

int nvme_register_write(struct nvm_dev *dev, uint32_t regaddr, uint32_t value)
{
	int err = 0;
	struct nvme_misc_cmd ctl;

	memset(&ctl, 0, sizeof(ctl));
	ctl.cmdtype = NVME_MISC_WR_REGISTER;
	ctl.regaddr = regaddr;
	ctl.val32 = value;
	
	err = ioctl(dev->fd, NVME_IOCTL_MISCTOOL, &ctl);
	if (err)
		printf("rdreg retrun invalid");

	return err;
}

