#include <linux/lightnvm.h>

/* COPY-PASTE FROM dflash.h -- BEGIN */
struct nvm_ioctl_vblock {
	__u64 ppa;
	__u16 flags;
	__u16 rsvd[3];
};

struct nvm_ioctl_io
{
	__u8 opcode;
	__u8 flags;
	__u16 nppas;
	__u32 rsvd2;
	__u64 metadata;
	__u64 addr;
	__u64 ppas;
	__u32 metadata_len;
	__u32 data_len;
	__u64 status;
	__u32 result;
	__u32 rsvd3[3];
};

/* TODO Make commands reserved in the global lightnvm ioctl opcode pool */
enum {
	/* Provisioning interface */
	NVM_BLOCK_GET_CMD = 0x40,
	NVM_BLOCK_PUT_CMD,

	/* IO Interface */
	NVM_PIO_CMD,
};

#define NVM_BLOCK_GET		_IOWR(NVM_IOCTL, NVM_BLOCK_GET_CMD, \
						struct nvm_ioctl_vblock)
#define NVM_BLOCK_PUT		_IOWR(NVM_IOCTL, NVM_BLOCK_PUT_CMD, \
						struct nvm_ioctl_vblock)
#define NVM_PIO			_IOWR(NVM_IOCTL, NVM_PIO_CMD, \
						struct nvm_ioctl_io)
/* COPY-PASTE FROM dflash.h -- BEGIN */

