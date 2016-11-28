/*
 * User space I/O library for Open-channel SSDs
 *
 * Copyright (C) 2015 Javier González <javier@cnexlabs.com>
 * Copyright (C) 2015 Matias González <javier@cnexlabs.com>
 * Copyright (C) 2016 Simon A. F. Lund <slund@cnexlabs.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __LIBLIGHTNVM_H
#define __LIBLIGHTNVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>

#define NVM_DEV_NAME_LEN 32
#define NVM_DEV_PATH_LEN (NVM_DEV_NAME_LEN + 5)

#define NVM_MAGIC_FLAG_SNGL 0x0		///< Single-plane
#define NVM_MAGIC_FLAG_DUAL 0x1		///< Dual-plane (NVM_IO_DUAL_ACCESS)
#define NVM_MAGIC_FLAG_QUAD 0x2		///< Quad-plane (NVM_IO_QUAD_ACCESS)
#define NVM_MAGIC_FLAG_SCRBL 0x200	///< Scrambler ON/OFF: Context sensitive

#define NVM_MAGIC_FLAG_DEFAULT (NVM_MAGIC_FLAG_SNGL | NVM_MAGIC_FLAG_SCRBL);

#define NVM_BLK_BITS (16)	///< Number of bits for block field
#define NVM_PG_BITS  (16)	///< Number of bits for page field
#define NVM_SEC_BITS (8)	///< Number of bits for sector field
#define NVM_PL_BITS  (8)	///< Number of bits for plane field
#define NVM_LUN_BITS (8)	///< Number of bits for lun field
#define NVM_CH_BITS  (7)	///< NUmber of bits for channel field

/**
 * Encapsulation of generic physical nvm addressing
 *
 * The kernel translates the generic physical address represented by this
 * structure to device specific address formats. Although the user need not
 * worry about device specific address formats the user has to know and respect
 * addressing within device specific geometric boundaries.
 *
 * For that purpose one can use the NVM_GEO of an NVM_DEV to obtain device
 * specific geometries.
 *
 * @ingroup NVM_ADDR
 */
typedef struct nvm_addr {
	union {
		/**
		 * Address packing and geometric accessors
		 */
		struct {
			uint64_t blk	: NVM_BLK_BITS;	///< Block address
			uint64_t pg	: NVM_PG_BITS;	///< Page address
			uint64_t sec	: NVM_SEC_BITS;	///< Sector address
			uint64_t pl	: NVM_PL_BITS;	///< Plane address
			uint64_t lun	: NVM_LUN_BITS;	///< Lun address
			uint64_t ch	: NVM_CH_BITS;	///< Channel address
			uint64_t rsvd	: 1;		///< Reserved
		} g;

		struct {
			uint64_t line        : 63;	///< Address line
			uint64_t is_cached   : 1;	///< Cache hint?
		} c;

		uint64_t ppa;				///< Address as ppa
	};
} NVM_ADDR;

/**
 * Encoding descriptor for address formats
 *
 * @ingroup NVM_ADDR
 */
typedef struct nvm_addr_fmt {
	union {
		/**
		 * Address formed as named fields
		 */
		struct {
			uint8_t ch_ofz;   ///< Offset in bits for channel
			uint8_t ch_len;   ///< Nr. of bits representing channel
			uint8_t lun_ofz;  ///< Offset in bits for lun
			uint8_t lun_len;  ///< Nr. of bits representing lun
			uint8_t pl_ofz;   ///< Offset in bits for plane
			uint8_t pl_len;   ///< Nr. of bits representing plane
			uint8_t blk_ofz;  ///< Offset in bits for block
			uint8_t blk_len;  ///< Nr. of bits representing block
			uint8_t pg_ofz;   ///< Offset in bits for page
			uint8_t pg_len;   ///< Nr. of bits representing page
			uint8_t sec_ofz;  ///< Offset in bits for sector
			uint8_t sec_len;  ///< Nr. of bits representing sector
		} n;

		/**
		 * Address formed as anonymous consecutive fields
		 */
		uint8_t a[12];
	};
} NVM_ADDR_FMT;


/**
 * Representation of geometry of devices and spanning blocks.
 *
 * @ingroup NVM_GEO
 */
typedef struct nvm_geo {
	size_t nchannels;	///< Number of channels on device
	size_t nluns;		///< Number of luns per channel
	size_t nplanes;		///< Number of planes per lun
	size_t nblocks;		///< Number of blocks per plane
	size_t npages;		///< Number of pages per block
	size_t nsectors;	///< Number of sectors per page
	size_t nbytes;		///< Number of bytes per sector

	size_t tbytes;		///< Total number of bytes on device
	size_t vblk_nbytes;	///< Number of bytes per virtual block
	size_t vpg_nbytes;	///< Number of bytes per virtual page
} NVM_GEO;

/**
 * Handle for nvm devices
 *
 * @ingroup NVM_DEV
 */
typedef struct nvm_dev *NVM_DEV;

/**
 * Virtual blocks facilitate a libc-like write, pwrite, read, pread
 * interface to perform I/O on blocks on nvm.
  *
 * @ingroup NVM_VBLK
 */
typedef struct nvm_vblk *NVM_VBLK;

/**
 * Spanning block
 *
 * @ingroup NVM_SBLK
 */
typedef struct nvm_sblk *NVM_SBLK;

/**
 * Prints human readable representation of given geometry
 *
 * @ingroup NVM_GEO
 */
void nvm_geo_pr(NVM_GEO geo);

/**
 * Creates a handle to given device path
 *
 * @ingroup NVM_DEV
 * @param dev_path Path of the device to open e.g. "/dev/nvme0n1"
 * @returns A handle to the device
 */
NVM_DEV nvm_dev_open(const char *dev_path);

/**
 * Destroys device-handle
 *
 * @ingroup NVM_DEV
 * @param dev Handle to destroy
 */
void nvm_dev_close(NVM_DEV dev);

/**
 * Prints information about the device associated with the given handle
 *
 * @param dev Handle of the device to print information about
 */
void nvm_dev_pr(NVM_DEV dev);

/**
 * Returns the geometry of the given device
 *
 * NOTE: See NVM_GEO for the specifics
 *
 * @param dev The device to obtain the geometry of
 * @returns The geometry (NVM_GEO) of given device handle
 */
NVM_GEO nvm_dev_attr_geo(NVM_DEV dev);

/**
 * Allocate a buffer aligned to match the given geometry
 *
 * @param geo The geometry to get alignment information from
 * @param nbytes The size of the allocated buffer in bytes
 * @returns On succes: a pointer to the allocated memory. On error: NULL.
 */
void *nvm_buf_alloc(NVM_GEO geo, size_t nbytes);

/**
 * Fills `buf` with chars A-Z
 *
 * @param buf Pointer to the buffer to fill
 * @param nbytes Amount of bytes to fill in buf
 */
void nvm_buf_fill(char *buf, size_t nbytes);

/**
 * Prints `buf` to stdout
 *
 * @param buf Pointer to the buffer to print
 * @param nbytes Amount of bytes of buf to print
 */
void nvm_buf_pr(char *buf, size_t nbytes);

/**
 * Mark addresses good, bad, or host-bad.
 *
 * NOTE: The addresses given to this function are interpreted as block
 *       addresses, in contrast to nvm_addr_mark, nvm_addr_write, and
 *       nvm_addr_read for which the address is interpreted as a sector address.
 *
 * @param dev Handle to the device on which to mark
 * @param list List of memory address
 * @param len Length of memory address list
 * @param flags 0x0 = GOOD, 0x1 = BAD, 0x2 = GROWN_BAD, as well as access mode
 * @returns On success: 0. On error: -1 and errno set accordingly.
 */
ssize_t nvm_addr_mark(NVM_DEV dev, NVM_ADDR list[], int len, uint16_t flags);

/**
 * Erase nvm at given addresses
 *
 * NOTE: The addresses given to this function are interpreted as block
 *       addresses, in contrast to nvm_addr_mark, nvm_addr_write, and
 *       nvm_addr_read for which the address is interpreted as a sector address.
 *
 * @param dev Handle to the device on which to erase
 * @param list List of memory address
 * @param len Length of memory address list
 * @param flags Access mode
 * @returns On success: 0. On error: -1 and errno set accordingly.
 */
ssize_t nvm_addr_erase(NVM_DEV dev, NVM_ADDR list[], int len, uint16_t flags);

/**
 * Write content of buf to nvm at address(es)
 *
 * NOTE: The addresses given to this function are interpreted as sector
 *       addresses, in contrast to nvm_addr_mark and nvm_addr_erase for which
 *       the address is interpreted as a block address.
 *
 * @param dev Handle to the device on which to erase
 * @param list List of memory address
 * @param len Length of memory address list
 * @param buf The buffer which content to write
 * @param flags Access mode
 * @returns On success: 0. On error: -1 and errno set accordingly.
 */
ssize_t nvm_addr_write(NVM_DEV dev, NVM_ADDR list[], int len, const void *buf,
                       uint16_t flags);

/**
 * Read content of nvm at addresses into buf
 *
 * NOTE: The addresses given to this function are interpreted as sector
 *       addresses, in contrast to nvm_addr_mark and nvm_addr_erase for which
 *       the address is interpreted as a block address.
 *
 * @param dev Handle to the device on which to erase
 * @param list List of memory address
 * @param len Length of memory address list
 * @param buf The buffer which content to write
 * @param flags Access mode
 * @returns On success: 0. On error: -1 and errno set accordingly.
 */
ssize_t nvm_addr_read(NVM_DEV dev, NVM_ADDR list[], int len, void *buf,
                      uint16_t flags);

/**
 * Prints a humanly readable representation of the given address
 *
 * @param addr The address to print
 */
void nvm_addr_pr(NVM_ADDR addr);

/**
 * Prints a humanly readable representation of the give address format
 *
 * @param fmt The address format to porint
 */
void nvm_addr_fmt_pr(NVM_ADDR_FMT* fmt);

NVM_VBLK nvm_vblk_new(void);
NVM_VBLK nvm_vblk_new_on_dev(NVM_DEV dev, NVM_ADDR addr);
void nvm_vblk_free(NVM_VBLK vblk);
void nvm_vblk_pr(NVM_VBLK vblk);

NVM_ADDR nvm_vblk_attr_addr(NVM_VBLK vblk);

/**
 * Get ownership of an arbitrary flash block from the given device.
 *
 * Returns: On success, a flash block is allocated in LightNVM's media manager
 * and vblk is filled up accordingly. On error, -1 is returned, in which case
 * errno is set to indicate the error.
 */
int nvm_vblk_get(NVM_VBLK vblk, NVM_DEV dev);

/**
 * Reserves a block on given device using a specific lun.
 *
 * @param vblk Block created with nvm_vblk_new
 * @param dev Handle obtained with nvm_dev_open
 * @param ch Channel from which to reserve via
 * @param lun Lun from which to reserve via
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
int nvm_vblk_gets(NVM_VBLK vblk, NVM_DEV dev, uint32_t ch, uint32_t lun);

/**
 * Put flash block(s) represented by vblk back to dev.
 *
 * This action implies that the owner of the flash block previous to this
 * function call no longer owns the flash block, and therefor can no longer
 * submit I/Os to it, or expect that data on it is persisted. The flash block
 * cannot be reclaimed by the previous owner.
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
int nvm_vblk_put(NVM_VBLK vblk);

/**
 * Erase an entire vblk
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_erase(NVM_VBLK vblk);

/**
 * Read 'count' bytes from 'vblk' starting at 'offset' into 'buf'
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_pread(NVM_VBLK vblk, void *buf, size_t count, size_t offset);

/**
 * Write 'count' bytes to 'vblk' starting at 'offset' from 'buf'
 *
 * NOTE: Use this for controlling chunked writing, do NOT use this for
 *       random-access.
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_pwrite(NVM_VBLK vblk, const void *buf, size_t count,
			size_t offset);

/**
 * Write 'count' bytes to 'vblk' starting at 'offset' from 'buf'
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_write(NVM_VBLK vblk, const void *buf, size_t count);

/**
 * Read the entire vblk, storing it into buf
 *
 * @returns On success 0, on error -1 and *errno* set appropriately.
 */
ssize_t nvm_vblk_read(NVM_VBLK vblk, void *buf, size_t count);


/**
 * Create a new sblk.
 */
NVM_SBLK nvm_sblk_new(NVM_DEV dev, int ch_bgn, int ch_end, int lun_bgn,
                      int lun_end, int blk);

/**
 * Destroy an sblk
 */
void nvm_sblk_free(NVM_SBLK sblk);

/**
 * Erase an sblk
 */
ssize_t nvm_sblk_erase(NVM_SBLK sblk);

/**
 * Write content from buffer to sblk
 */
ssize_t nvm_sblk_write(NVM_SBLK sblk, const void *buf, size_t count);

/**
 * Pad the sblk
 */
ssize_t nvm_sblk_pad(NVM_SBLK sblk);

/**
 * Read content from sblk to buffer
 */
ssize_t nvm_sblk_read(NVM_SBLK sblk, void *buf, size_t count);

/**
 * Positioned read from sblk to buffer
 */
ssize_t nvm_sblk_pread(struct nvm_sblk *sblk, void *buf, size_t count,
		       size_t offset);

NVM_DEV nvm_sblk_attr_dev(NVM_SBLK sblk);
NVM_ADDR nvm_sblk_attr_bgn(NVM_SBLK sblk);
NVM_ADDR nvm_sblk_attr_end(NVM_SBLK sblk);
NVM_GEO nvm_sblk_attr_geo(NVM_SBLK sblk);
void nvm_sblk_pr(NVM_SBLK sblk);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
