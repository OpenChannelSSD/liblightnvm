/*
 * User space I/O library for Open-Channel SSDs
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

#define NVM_FLAG_PMODE_SNGL 0x0	///< Single-plane
#define NVM_FLAG_PMODE_DUAL 0x1	///< Dual-plane (NVM_IO_DUAL_ACCESS)
#define NVM_FLAG_PMODE_QUAD 0x2	///< Quad-plane (NVM_IO_QUAD_ACCESS)
#define NVM_FLAG_SCRBL 0x200	///< Scrambler ON/OFF: Context sensitive

#define NVM_FLAG_DEFAULT (NVM_FLAG_PMODE_SNGL | NVM_FLAG_SCRBL);

#define NVM_BLK_BITS (16)	///< Number of bits for block field
#define NVM_PG_BITS  (16)	///< Number of bits for page field
#define NVM_SEC_BITS (8)	///< Number of bits for sector field
#define NVM_PL_BITS  (8)	///< Number of bits for plane field
#define NVM_LUN_BITS (8)	///< Number of bits for lun field
#define NVM_CH_BITS  (7)	///< NUmber of bits for channel field

/**
 * Opaque handle for NVM devices
 *
 * @struct nvm_dev
 */
struct nvm_dev;

/**
 * Deprecated block interface
 *
 * @struct nvm_dblk
 */
struct nvm_dblk;

/**
 * Virtual block abstraction
 *
 * Facilitates a libc-like read/write and a system-like `pread`/`pwrite`
 * interface to perform I/O on multiple blocks on physical NVM. Span defaults to
 * multiple physical blocks across planes when allocated with `nvm_dblk_alloc`.
 * Span of multiple physical blocks accross LUNs, and channels can be created
 * when using `nvm_dblk_alloc_span`.
 *
 * @struct nvm_sblk
 */
struct nvm_sblk;

enum nvm_bounds {
	NVM_BOUNDS_CHANNEL = 1,
	NVM_BOUNDS_LUN = 2,
	NVM_BOUNDS_PLANE = 4,
	NVM_BOUNDS_BLOCK = 8,
	NVM_BOUNDS_PAGE = 16,
	NVM_BOUNDS_SECTOR = 32
};

/**
 * Encapsulation and representation of lower-level error conditions
 */
struct nvm_ret {
	uint64_t status;	///< NVMe command status / completion bits
	uint32_t result;	///< NVMe command error codes
};

/**
 * Encapsulation of generic physical nvm addressing
 *
 * The kernel translates the generic physical address represented by this
 * structure to device specific address formats. Although the user need not
 * worry about device specific address formats the user has to know and respect
 * addressing within device specific geometric boundaries.
 *
 * For that purpose one can use the `struct nvm_geo` of an `struct nvm_dev` to
 * obtain device specific geometries.
 */
struct nvm_addr {
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
			uint64_t line		: 63;	///< Address line
			uint64_t is_cached	: 1;	///< Cache hint?
		} c;

		uint64_t ppa;				///< Address as ppa
	};
};

/**
 * Encoding descriptor for address formats
 */
struct nvm_addr_fmt {
	union {
		/**
		 * Address format formed as named fields
		 */
		struct {
			uint8_t ch_ofz;		///< Offset in bits for channel
			uint8_t ch_len;		///< Nr. of bits repr. channel
			uint8_t lun_ofz;	///< Offset in bits for lun
			uint8_t lun_len;	///< Nr. of bits repr. lun
			uint8_t pl_ofz;		///< Offset in bits for plane
			uint8_t pl_len;		///< Nr. of bits repr. plane
			uint8_t blk_ofz;	///< Offset in bits for block
			uint8_t blk_len;	///< Nr. of bits repr. block
			uint8_t pg_ofz;		///< Offset in bits for page
			uint8_t pg_len;		///< Nr. of bits repr. page
			uint8_t sec_ofz;	///< Offset in bits for sector
			uint8_t sec_len;	///< Nr. of bits repr. sector
		} n;

		/**
		 * Address format formed as anonymous consecutive fields
		 */
		uint8_t a[12];
	};
};

/**
 * Representation of geometry of devices and spanning blocks.
 */
struct nvm_geo {
	size_t nchannels;	///< Number of channels on device
	size_t nluns;		///< Number of luns per channel
	size_t nplanes;		///< Number of planes per lun
	size_t nblocks;		///< Number of blocks per plane
	size_t npages;		///< Number of pages per block
	size_t nsectors;	///< Number of sectors per page

	size_t page_nbytes;	///< Number of bytes per page
	size_t sector_nbytes;	///< Number of bytes per sector
	size_t meta_nbytes;	///< Number of bytes for out-of-bound / metadata

	size_t tbytes;		///< Total number of bytes in geometry
	size_t vblk_nbytes;	///< Number of bytes per virtual block
	size_t vpg_nbytes;	///< Number of bytes per virtual page
};

enum nvm_bbt_mark {
	NVM_BBT_GOOD = 0x0,	///< Block is free / good
	NVM_BBT_BAD = 0x1,	///< Block is bad
	NVM_BBT_GBAD = 0x2	///< Block has grown bad
};

/**
 * Representation of bad-block-table
 *
 * Create and initialize by calling `nvm_bbt_get`
 * Destroy by calling `free`
 * Print it with `nvm_bbt_pr`
 * Update it by accessing blks[] after `nvm_bbt_get`
 * Update device by calling `nvm_bbt_set`
 */
struct nvm_bbt {
	struct nvm_dev *dev;	///< Device on which the bbt belongs to
	struct nvm_addr addr;	///< LUN address the bad block table covers
	uint8_t *blks;	///< Array containing block status for each block in LUN
	uint64_t nblks;	///< Length of the bad block array
};

/**
 * @returns the "major" version of the library
 */
int nvm_ver_major(void);

/**
 * @returns the "minor" version of the library
 */
int nvm_ver_minor(void);

/**
 * @returns the "patch" version of the library
 */
int nvm_ver_patch(void);

/**
 * Prints version information about the library
 */
void nvm_ver_pr(void);

/**
 * Prints a humanly readable description of given boundary mask
 */
void nvm_bounds_pr(int mask);

/**
 * Checks whether the given addr exceeds bounds of the given geometry
 *
 * @param addr The addr to check
 * @param geo The geometric bounds to check the given address against
 * @returns A mask of exceeded boundaries
 */
int nvm_addr_check(struct nvm_addr addr, const struct nvm_geo *geo);

/**
 * Convert nvm_address from generic format to device specific format
 *
 * @param dev The device which address format to convert to
 * @param addr The address to convert
 * @returns Address formatted to device
 */
struct nvm_addr nvm_addr_gen2dev(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Maps the given generically formatted physical address to LBA offset.
 */
size_t nvm_addr_gen2lba(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Maps the given LBA offset to the corresponding generically formatted physical
 * address.
 */
struct nvm_addr nvm_addr_lba2gen(struct nvm_dev *dev, size_t lba);

/**
 * Read up to `count` bytes from the given `device` starting at the given
 * `offset` into the given buffer starting at `buf`.
 *
 * @note
 * This is equivalent to `pread`/`pwrite` except it takes the opaque `struct
 * nvm_dev *` instead of a file descriptor
 */
ssize_t nvm_lba_pread(struct nvm_dev *dev, void *buf, size_t count,
		      off_t offset);

/**
 * Write up to `count` bytes from the buffer starting at `buf` to the given
 * device `dev` at given `offset`.
 *
 * @note
 * This is equivalent to `pread`/`pwrite` except it takes the opaque `struct
 * nvm_dev *` instead of a file descriptor
 */
ssize_t nvm_lba_pwrite(struct nvm_dev *dev, const void *buf, size_t count,
		       off_t offset);

/**
 * Prints a humanly readable representation the given `struct nvm_ret`
 *
 * @param ret Pointer to the `struct nvm_ret` to print
 */
void nvm_ret_pr(struct nvm_ret *ret);

/**
 * Retrieves a bad block table from device
 *
 * @param dev The device on which to retrieve a bad-block-table from
 * @param addr Address of the LUN to retrieve bad-block-table for
 * @param ret Pointer to structure in which to store lower-level status and
 *            result.
 * @returns On success, a pointer to the bad-block-table is returned. On error,
 * NULL is returned, `errno` set to indicate the error and ret filled with
 * lower-level result codes
 */
struct nvm_bbt* nvm_bbt_get(struct nvm_dev *dev, struct nvm_addr addr,
			    struct nvm_ret *ret);

/**
 * Updates the bad-block-table on given device using the provided bbt
 *
 * @param dev The device on which to update a bad-block-table
 * @param bbt The bbt to write to device
 * @param ret Pointer to structure in which to store lower-level status and
 *            result.
 * @returns On success, the number of updated entries is returned. On error, -1
 * is returned, `errno` set to indicate the error and ret filled with with
 * lower-level result codes
 */
int nvm_bbt_set(struct nvm_dev *dev, struct nvm_bbt* bbt, struct nvm_ret *ret);

/**
 * Mark addresses good, bad, or host-bad.
 *
 * @note
 * The addresses given to this function are interpreted as block addresses, in
 * contrast to `nvm_addr_write`, and `nvm_addr_read` which interpret addresses
 * and sector addresses.
 *
 * @param dev Handle to the device on which to mark
 * @param addrs Array of memory address
 * @param naddrs Length of memory address array
 * @param flags 0x0 = GOOD, 0x1 = BAD, 0x2 = GROWN_BAD, as well as access mode
 * @param ret Pointer to structure in which to store lower-level status and
 *            result.
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_mark(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		 uint16_t flags, struct nvm_ret *ret);

/**
 * Destroys a given bad-block-table
 *
 * @param bbt The bad-block-table to destroy
 */
void nvm_bbt_free(struct nvm_bbt *bbt);

/**
 * Prints a humanly readable representation of the given bad-block-table
 *
 * @param bbt The bad-block-table to print
 */
void nvm_bbt_pr(struct nvm_bbt *bbt);

/**
 * Prints human readable representation of the given geometry
 */
void nvm_geo_pr(const struct nvm_geo *geo);

/**
 * Creates a handle to given device path
 *
 * @param dev_path Path of the device to open e.g. "/dev/nvme0n1"
 *
 * @returns A handle to the device
 */
struct nvm_dev * nvm_dev_open(const char *dev_path);

/**
 * Destroys device-handle
 *
 * @param dev Handle to destroy
 */
void nvm_dev_close(struct nvm_dev *dev);

/**
 * Prints information about the device associated with the given handle
 *
 * @param dev Handle of the device to print information about
 */
void nvm_dev_pr(struct nvm_dev *dev);

/**
 * Returns the default plane_mode of the given device
 *
 * @param dev The device to obtain the default plane mode for
 * @return On success, pmode flag is returned.
 */
int nvm_dev_attr_pmode(struct nvm_dev *dev);

/**
 * Returns the geometry of the given device
 *
 * @note
 * See struct nvm_geo for the specifics of the returned geometry
 *
 * @param dev The device to obtain the geometry of
 *
 * @returns The geometry (struct nvm_geo) of given device handle
 */
const struct nvm_geo *nvm_dev_attr_geo(struct nvm_dev *dev);

/**
 * Allocate a buffer aligned to match the given geometry
 *
 * @note
 * nbytes must be greater than zero and a multiple of geo.vpage_nbytes
 *
 * @param geo The geometry to get alignment information from
 * @param nbytes The size of the allocated buffer in bytes
 *
 * @returns A pointer to the allocated memory. On error: NULL is returned and
 * `errno` set appropriatly
 */
void *nvm_buf_alloc(const struct nvm_geo *geo, size_t nbytes);

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
 * Erase nvm at given addresses
 *
 * @note
 * The addresses given to this function are interpreted as block addresses, in
 * contrast to `nvm_addr_mark`, `nvm_addr_write`, and `nvm_addr_read` for which
 * the address is interpreted as a sector address.
 *
 * @param dev Handle to the device on which to erase
 * @param addrs Array of memory address
 * @param naddrs Length of array of memory addresses
 * @param flags Access mode
 * @param ret Pointer to structure in which to store lower-level status and
 *            result.
 * @returns 0 on success. On error: returns -1, sets `errno` accordingly, and
 *          fills `ret` with lower-level result and status codes
 */
ssize_t nvm_addr_erase(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       uint16_t flags, struct nvm_ret *ret);

/**
 * Write content of buf to nvm at address(es)
 *
 * @note
 * The addresses given to this function are interpreted as sector addresses, in
 * contrast to nvm_addr_mark and nvm_addr_erase for which the address is
 * interpreted as a block address.
 *
 * @param dev Handle to the device on which to erase
 * @param addrs Array of memory address
 * @param naddrs Length of array of memory addresses
 * @param buf The buffer which content to write, must be aligned to device
 *            geo.vpage_nbytes and size equal to `naddrs * geo.nbytes`
 * @param meta Buffer containing metadata, must be of size equal to device
 *             `naddrs * geo.meta_nbytes`
 * @param flags Access mode
 * @param ret Pointer to structure in which to store lower-level status and
 *            result.
 * @returns 0 on success. On error: returns -1, sets `errno` accordingly, and
 *          fills `ret` with lower-level result and status codes
 */
ssize_t nvm_addr_write(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		       const void *buf, const void *meta, uint16_t flags,
		       struct nvm_ret *ret);

/**
 * Read content of nvm at addresses into buf
 *
 * @note
 * The addresses given to this function are interpreted as sector addresses, in
 * contrast to `nvm_addr_mark` and `nvm_addr_erase` for which the address is
 * interpreted as a block address.
 *
 * @param dev Handle to the device on which to erase
 * @param addrs List of memory address
 * @param naddrs Length of array of memory addresses
 * @param buf Buffer to store result of read into, must be aligned to device
 *            geo.vpage_nbytes and size equal to `naddrs * geo.nbytes`
 * @param meta Buffer to store content of metadata, must be of size equal to
 *             device `naddrs * geo.meta_nbytes`
 * @param flags Access mode
 * @param ret Pointer to structure in which to store lower-level status and
 *            result.
 * @returns 0 on success. On error: returns -1, sets `errno` accordingly, and
 *          fills `ret` with lower-level result and status codes
 */
ssize_t nvm_addr_read(struct nvm_dev *dev, struct nvm_addr addrs[], int naddrs,
		      void *buf, void *meta, uint16_t flags,
		      struct nvm_ret *ret);

/**
 * Prints a humanly readable representation of the given address
 *
 * @param addr The address to print
 */
void nvm_addr_pr(struct nvm_addr addr);

/**
 * Prints a humanly readable representation of the give address format
 *
 * @param fmt The address format to porint
 */
void nvm_addr_fmt_pr(struct nvm_addr_fmt* fmt);

struct nvm_dblk *nvm_dblk_new(void);
struct nvm_dblk *nvm_dblk_alloc(struct nvm_dev *dev,
                                           struct nvm_addr addr);
struct nvm_dblk *nvm_dblk_alloc_span(struct nvm_dev *dev, int ch_bgn,
                                                int ch_end, int lun_bgn,
                                                int lun_end,
                                                int blk);
void nvm_dblk_free(struct nvm_dblk *dblk);
void nvm_dblk_pr(struct nvm_dblk *dblk);

struct nvm_addr nvm_dblk_attr_addr(struct nvm_dblk *dblk);

/**
 * Erase an entire dblk
 *
 * @returns On success, the number of bytes erased is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_dblk_erase(struct nvm_dblk *dblk);

/**
 * Read from a dblk at given offset
 *
 * @returns On success, the number of bytes read is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_dblk_pread(struct nvm_dblk *dblk, void *buf,
                                  size_t count,
                                  size_t offset);

/**
 * Write to a dblk at given offset
 *
 * @note
 * Use this for controlling "chunked" writing, do NOT use for random access
 *
 * @returns On success, the number of bytes written is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_dblk_pwrite(struct nvm_dblk *dblk, const void *buf,
                                   size_t count,
                                   size_t offset);

/**
 * Write to dblk
 *
 * @returns On success, the number of bytes written is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_dblk_write(struct nvm_dblk *dblk, const void *buf,
                                  size_t count);

/**
 * Read from a dblk
 *
 * @returns On success, the number of bytes read is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_dblk_read(struct nvm_dblk *dblk, void *buf, size_t count);

/**
 * Allocate an sblk and initialize it
 *
 * @param dev The device on which the sblk resides
 * @param addr Address of the virtual block
 *
 * @returns On success, an opaque pointer to the initialized sblk is returned.
 * On error, NULL and `errno` set to indicate the error.
 */
struct nvm_sblk *nvm_sblk_alloc(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Allocate an sblk and initialize it
 *
 * @param dev The device on which the sblk resides
 * @param ch_bgn Beginning of the channel span, as inclusive index
 * @param ch_end End of the channel span, as inclusive index
 * @param lun_bgn Beginning of the lun span, as inclusive index
 * @param lun_end End of the lun span, as inclusive index
 * @param blk Block index
 *
 * @returns On success, an opaque pointer to the initialized sblk is returned.
 * On error, NULL and `errno` set to indicate the error.
 */
struct nvm_sblk *nvm_sblk_alloc_span(struct nvm_dev *dev, int ch_bgn,
                                     int ch_end,
                                     int lun_bgn, int lun_end, int blk);

/**
 * Destroy an sblk
 *
 * @param sblk The sblk to destroy
 */
void nvm_sblk_free(struct nvm_sblk *sblk);

/**
 * Erase an sblk
 *
 * @param sblk The sblk to erase
 * @returns On success, the number of bytes erased is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_sblk_erase(struct nvm_sblk *sblk);

/**
 * Write to an sblk
 *
 * @note
 * buf must be aligned to device geometry, see struct nvm_geo and nvm_buf_alloc
 * count must be a multiple of min-size, see struct nvm_geo
 * do not mix use of nvm_sblk_pwrite with nvm_sblk_write on the same sblk
 *
 * @param sblk The sblk to write to
 * @param buf Write content starting at buf
 * @param count The number of bytes to write
 * @returns On success, the number of bytes written is returned and sblk
 * internal position is updated. On error, -1 is returned and `errno` set to
 * indicate the error.
 */
ssize_t nvm_sblk_write(struct nvm_sblk *sblk, const void *buf, size_t count);

/**
 * Write to an sblk at a given offset
 *
 * @note
 * buf must be aligned to device geometry, see struct nvm_geo and nvm_buf_alloc
 * count must be a multiple of min-size, see struct nvm_geo
 * offset must be a multiple of min-size, see struct nvm_geo
 * do not mix use of nvm_sblk_pwrite with nvm_sblk_write on the same sblk
 *
 * @param sblk The sblk to write to
 * @param buf Write content starting at buf
 * @param count The number of bytes to write
 * @param offset Start writing offset bytes within sblk
 * @returns On success, the number of bytes written is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_sblk_pwrite(struct nvm_sblk *sblk, const void *buf, size_t count,
			size_t offset);

/**
 * Pad the sblk with synthetic data
 *
 * @note
 * Assumes that you have used nvm_sblk_write and now want to fill the remaining
 * sblk in order to meet sblk constraints
 *
 * @param sblk The sblk to pad
 * @returns On success, the number of bytes padded is returned and sblk internal
 * position is updated. On error, -1 is returned and `errno` set to indicate the
 * error.
 */
ssize_t nvm_sblk_pad(struct nvm_sblk *sblk);

/**
 * Read from an sblk
 */
ssize_t nvm_sblk_read(struct nvm_sblk *sblk, void *buf, size_t count);

/**
 * Read from an sblk at given offset
 */
ssize_t nvm_sblk_pread(struct nvm_sblk *sblk, void *buf, size_t count,
		       size_t offset);

/**
 * Retrieve the device associated with the given sblk
 *
 * @param sblk The entity to retrieve information from
 */
struct nvm_dev *nvm_sblk_attr_dev(struct nvm_sblk *sblk);

/**
 * Retrieve the address where the sblk begins
 *
 * @param sblk The entity to retrieve information from
 */
struct nvm_addr nvm_sblk_attr_bgn(struct nvm_sblk *sblk);

/**
 * Retrieve the address where the sblk ends
 *
 * @param sblk The entity to retrieve information from
 */
struct nvm_addr nvm_sblk_attr_end(struct nvm_sblk *sblk);

/**
 * Retrieve the geometry of the given sblk
 *
 * @param sblk The entity to retrieve information from
 */
const struct nvm_geo *nvm_sblk_attr_geo(struct nvm_sblk *sblk);

/**
 * Retrieve the current cursor position for writes to the sblk
 *
 * @param sblk The entity to retrieve information from
 */
size_t nvm_sblk_attr_pos_write(struct nvm_sblk *sblk);

/**
 * Retrieve the current cursor position for read to the sblk
 *
 * @param sblk The entity to retrieve information from
 */
size_t nvm_sblk_attr_pos_read(struct nvm_sblk *sblk);

/**
 * Print the sblk in a humanly readable form
 *
 * @param sblk The entity to print information about
 */
void nvm_sblk_pr(struct nvm_sblk *sblk);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
