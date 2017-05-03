/*
 * User space I/O library for Open-Channel SSDs
 *
 * Copyright (C) 2015-2017 Javier Gonzáles <javier@cnexlabs.com>
 * Copyright (C) 2015-2017 Matias Bjørling <matias@cnexlabs.com>
 * Copyright (C) 2015-2017 Simon A. F. Lund <slund@cnexlabs.com>
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
/**
 * @file liblightnvm.h
 */
#ifndef __LIBLIGHTNVM_H
#define __LIBLIGHTNVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>
#include <liblightnvm_spec.h>

#define NVM_NADDR_MAX 64

#define NVM_DEV_NAME_LEN 32
#define NVM_DEV_PATH_LEN (NVM_DEV_NAME_LEN + 5)

#define NVM_FLAG_SCRBL 0x200	///< Scrambler ON/OFF: Context sensitive

/**
 * Enumeration of cmd back-ends used by liblightnvm
 */
enum nvm_be_id {
	NVM_BE_ANY = 0x0,	///< Flag for ANY backend
	NVM_BE_IOCTL = 0x1,	///< Flag for the IOCTL backend
	NVM_BE_SYSFS = 0x2,	///< Flag for the IOCTL + sysfs backend
	NVM_BE_LBA = 0x4,	///< Flag for the IOCTL + sysfs + LBA backend
};
#define NVM_BE_ALL (NVM_BE_IOCTL | NVM_BE_SYSFS | NVM_BE_LBA)	///< All be identifiers

/**
 * Plane-mode access for IO
 */
enum nvm_pmode {
	NVM_FLAG_PMODE_SNGL = 0x0,	///< Single-plane
	NVM_FLAG_PMODE_DUAL = 0x1,	///< Dual-plane (NVM_IO_DUAL_ACCESS)
	NVM_FLAG_PMODE_QUAD = 0x2	///< Quad-plane (NVM_IO_QUAD_ACCESS)
};
#define NVM_FLAG_DEFAULT (NVM_FLAG_PMODE_SNGL | NVM_FLAG_SCRBL);

/**
 * Opaque handle for NVM devices
 *
 * @struct nvm_dev
 */
struct nvm_dev;

/**
 * Virtual block abstraction
 *
 * Facilitates a libc-like read/write and a system-like `pread`/`pwrite`
 * interface to perform I/O on a virtual block spanning multiple blocks of
 * physical NVM.
 *
 * Consult the `nvm_vblk_alloc`, `nvm_vblk_alloc_line` for the different spans
 *
 * @see nvm_vblk_alloc
 * @see nvm_vblk_alloc_line
 *
 * @struct nvm_vblk
 */
struct nvm_vblk;

/**
 * Enumeration of pseudo meta mode
 */
enum nvm_meta_mode {
	NVM_META_MODE_NONE = 0x0,
	NVM_META_MODE_ALPHA = 0x1,
	NVM_META_MODE_CONST = 0x2
};

/**
 * Enumeration of device bounds
 */
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
 * @struct nvm_cmd_vuser
 * @struct nvm_cmd_vadmin
 * @struct nvm_cmd_user
 * @struct nvm_cmd_admin
 */

/**
 * Encapsulation of lowest-level user and admin commands
 */
struct nvm_cmd {
	union {
		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t control;
			uint16_t nppas;
			uint16_t rsvd;
			uint64_t metadata;
			uint64_t addr;
			uint64_t ppa_list;
			uint32_t metadata_len;
			uint32_t data_len;
			uint64_t status;
			uint32_t result;
			uint32_t rsvd3[3];
		} vuser;	///< Common fields for vector user commands

		struct {
			uint8_t opcode;
			uint8_t flags;
			uint8_t rsvd[2];
			uint32_t nsid;
			uint32_t cdw2;
			uint32_t cdw3;
			uint64_t metadata;
			uint64_t addr;
			uint32_t metadata_len;
			uint32_t data_len;
			uint64_t ppa_list;
			uint16_t nppas;
			uint16_t control;
			uint32_t cdw13;
			uint32_t cdw14;
			uint32_t cdw15;
			uint64_t status;
			uint32_t result;
			uint32_t timeout_ms;
		} vadmin;	///< Common fields for vector admin commands

		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t rsvd1;
			uint32_t nsid;
			uint32_t cdw2;
			uint32_t cdw3;
			uint64_t metadata;
			uint64_t addr;
			uint32_t metadata_len;
			uint32_t data_len;
			uint32_t cdw10;
			uint32_t cdw11;
			uint32_t cdw12;
			uint32_t cdw13;
			uint32_t cdw14;
			uint32_t cdw15;
			uint32_t timeout_ms;
			uint32_t result;
		} admin;	///< Common fields for admin commands

		struct {
			uint8_t opcode;
			uint8_t flags;
			uint16_t control;
			uint16_t nblocks;
			uint16_t rsvd;
			uint64_t metadata;
			uint64_t addr;
			uint64_t slba;
			uint32_t dsmgmt;
			uint32_t reftag;
			uint16_t apptag;
			uint16_t appmask;
		} user;		///< Common fields for user commands

		uint32_t cdw[20];	///< Command as array of dwords
	};
};

/**
 * Obtain string representation of the given plane-mode
 *
 * @param pmode The plane-mode to obtain string representation of
 *
 * @returns On success, string representation of the given plane-mode. On error,
 * "UNKN".
 */
const char *nvm_pmode_str(int pmode);

/**
 * Execute an user command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_user(struct nvm_dev *dev, struct nvm_cmd *cmd, struct nvm_ret *ret);

/**
 * Execute an admin command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_admin(struct nvm_dev *dev, struct nvm_cmd *cmd,
		  struct nvm_ret *ret);

/**
 * Execute a vector user command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_vuser(struct nvm_dev *dev, struct nvm_cmd *cmd,
		  struct nvm_ret *ret);

/**
 * Execute a vector admin command on the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param cmd The command to execute
 * @param ret Pointer to struct to fill with lower-level result-codes
 *
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_cmd_vadmin(struct nvm_dev *dev, struct nvm_cmd *cmd,
		   struct nvm_ret *ret);

/**
 * Prints a text-representation of the given command
 *
 * @param cmd The command to print
 */
void nvm_cmd_pr(struct nvm_cmd *cmd);

/**
 * Prints a textual presentation of the vuser par of the given command
 *
 * @param cmd The command to print
 */
void nvm_cmd_vuser_pr(struct nvm_cmd *cmd);

/**
 * Encapsulation of generic physical nvm addressing
 *
 * Although the user need not worry about device specific address formats the
 * user has to know and respect addressing within device specific geometric
 * boundaries.
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
			uint64_t blk	: 16;	///< Block address
			uint64_t pg	: 16;	///< Page address
			uint64_t sec	: 8;	///< Sector address
			uint64_t pl	: 8;	///< Plane address
			uint64_t lun	: 8;	///< LUN address
			uint64_t ch	: 7;	///< Channel address
			uint64_t rsvd	: 1;	///< Reserved
		} g;

		struct {
			uint64_t line		: 63;	///< Address line
			uint64_t is_cached	: 1;	///< Cache hint?
		} c;

		uint64_t ppa;				///< Address as ppa
	};
};

/**
 * Representation of device and virtual block geometry
 *
 * @see nvm_dev_get_geo, nvm_vblk_get_geo
 */
struct nvm_geo {
	size_t nchannels;	///< Number of channels on device
	size_t nluns;		///< Number of LUNs per channel
	size_t nplanes;		///< Number of planes per LUN
	size_t nblocks;		///< Number of blocks per plane
	size_t npages;		///< Number of pages per block
	size_t nsectors;	///< Number of sectors per page

	size_t page_nbytes;	///< Number of bytes per page
	size_t sector_nbytes;	///< Number of bytes per sector
	size_t meta_nbytes;	///< Number of bytes for out-of-bound / metadata

	size_t tbytes;		///< Total number of bytes in geometry
};

/**
 * Representation of valid values of bad-block-table states
 */
enum nvm_bbt_state {
	NVM_BBT_FREE = 0x0,	///< Block is free AKA good
	NVM_BBT_BAD = 0x1,	///< Block is bad
	NVM_BBT_GBAD = 0x2,	///< Block has grown bad
	NVM_BBT_DMRK = 0x4,	///< Block has been marked by device side
	NVM_BBT_HMRK = 0x8	///< Block has been marked by host side
};

/**
 * Representation of bad-block-table
 *
 * The bad-block-table describes block-state of a given LUN
 *
 * @see nvm_bbt_get, nvm_bbt_set, nvm_bbt_mark, nvm_bbt_free, and nvm_bbt_pr
 */
struct nvm_bbt {
	struct nvm_dev *dev;	///< Device on which the bbt resides
	struct nvm_addr addr;	///< Address of the LUN described by the bbt
	uint64_t nblks;		///< Total # of blocks in lun
	uint32_t nbad;		///< # of manufacturer marked bad blocks
	uint32_t ngbad;		///< # of grown bad blocks
	uint32_t ndmrk;		///< # of device reserved/marked blocks
	uint32_t nhmrk;		///< # of of host reserved/marked blocks
	uint8_t blks[];		///< Array of block status for each block in LUN
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
void nvm_ret_pr(const struct nvm_ret *ret);

/**
 * Retrieves a bad block table from device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr Address of the LUN to retrieve bad-block-table for
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 * @returns On success, a pointer to the bad-block-table is returned. On error,
 * NULL is returned, `errno` set to indicate the error and ret filled with
 * lower-level result codes
 */
const struct nvm_bbt *nvm_bbt_get(struct nvm_dev *dev, struct nvm_addr addr,
				  struct nvm_ret *ret);

/**
 * Updates the bad-block-table on given device using the provided bbt
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param bbt The bbt to write to device
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_set(struct nvm_dev *dev, const struct nvm_bbt *bbt,
		struct nvm_ret *ret);

/**
 * Mark addresses good, bad, or host-bad.
 *
 * @note
 * The addresses given to this function are interpreted as block addresses, in
 * contrast to `nvm_addr_write`, and `nvm_addr_read` which interpret addresses
 * and sector addresses.
 *
 * @see `enum nvm_bbt_state`
 *
 * @param dev Device handle obtained with `nvm_dev_open`
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
 * Persist the bad-block-table at `addr` on device and deallocate managed memory
 * for the given bad-block-table describing the LUN at `addr`.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr Address of the LUN to flush bad-block-table for
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_flush(struct nvm_dev *dev, struct nvm_addr addr,
		  struct nvm_ret *ret);

/**
 * Persist all bad-block-tables associated with the given `dev`
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param ret Pointer to structure in which to store lower-level status and
 *            result
 * @returns On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate the error and ret filled with lower-level result codes
 */
int nvm_bbt_flush_all(struct nvm_dev *dev, struct nvm_ret *ret);

/**
 * Allocate a copy of the given bad-block-table
 *
 * @param bbt Pointer to the bad-block-table to copy
 * @returns On success, a pointer to a write-able copy of the given bbt is
 * returned. On error, NULL is returned and errno set to indicate the error
 */
struct nvm_bbt *nvm_bbt_alloc_cp(const struct nvm_bbt *bbt);

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
void nvm_bbt_pr(const struct nvm_bbt *bbt);

/**
 * Prints a humanly readable representation of the given bad-block-table state
 */
void nvm_bbt_state_pr(int state);

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
struct nvm_dev *nvm_dev_open(const char *dev_path);

/**
 * Creates a handle to given device path
 *
 * @param dev_path Path of the device to open e.g. "/dev/nvme0n1"
 * @param flags Flags for opening device in different modes
 *
 * @returns A handle to the device
 */
struct nvm_dev *nvm_dev_openf(const char *dev_path, int flags);

/**
 * Destroys device-handle
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
void nvm_dev_close(struct nvm_dev *dev);

/**
 * Prints misc. device attribute associated with the given handle
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
void nvm_dev_attr_pr(const struct nvm_dev *dev);

/**
 * Prints all information about the device associated with the given handle
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
void nvm_dev_pr(const struct nvm_dev *dev);

/**
 * Returns the file-descriptor associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, file descriptor is returned
 */
int nvm_dev_get_fd(const struct nvm_dev *dev);

/**
 * Returns the name associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, string is returned. On error, NULL is returned.
 */
const char *nvm_dev_get_name(const struct nvm_dev *dev);

/**
 * Returns the path associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, string is returned. On error, NULL is returned.
 */
const char *nvm_dev_get_path(const struct nvm_dev *dev);

/**
 * Returns the NVME namespace identifier of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @return On success, NVME namespace identifier is returned.
 */
int nvm_dev_get_nsid(const struct nvm_dev *dev);

/**
 * Returns the verid of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, verid is returned
 */
int nvm_dev_get_verid(const struct nvm_dev *dev);

/**
 * Returns the media-controller capabilities mask of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, capabilities mask is returned
 */
uint32_t nvm_dev_get_mccap(const struct nvm_dev *dev);

/**
 * Returns the default plane_mode of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, pmode flag is returned
 */
int nvm_dev_get_pmode(const struct nvm_dev *dev);

/**
 * Set the default plane-mode for the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param pmode Default plane-mode
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int nvm_dev_set_pmode(struct nvm_dev *dev, int pmode);

/**
 * Returns the ppa-format of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, ppa-format is returned
 *
 */
const struct nvm_spec_ppaf_nand *nvm_dev_get_ppaf(const struct nvm_dev *dev);

/**
 * Returns the ppa-format mask of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, ppa-format mask is returned
 *
 */
const struct nvm_spec_ppaf_nand_mask *nvm_dev_get_ppaf_mask(const struct nvm_dev *dev);

/**
 * Returns the 'meta-mode' of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @return On success, meta-mode is returned
 */
int nvm_dev_get_meta_mode(const struct nvm_dev *dev);

/**
 * Set the default 'meta-mode' of the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @returns On success, 0 is returned. On error, -1 is returned and errno set to
 * indicate the error.
 */
int nvm_dev_set_meta_mode(struct nvm_dev *dev, int meta_mode);

/**
 * Returns the maximum number of addresses to use when sending erases to device.
 * That is, when invoking nvm_addr_erase.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_erase_naddrs_max(const struct nvm_dev *dev);

/**
 * Returns whether caching is enabled for bad-block-tables on the device.
 *
 * @note
 * 0 = cache disabled
 * 1 = cache enabled
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_bbts_cached(const struct nvm_dev *dev);

/**
 * Returns the backend identifier associated with the given device
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_be_id(const struct nvm_dev *dev);

/**
 * Set the maximum number of addresses to use for reads, that is, when invoking
 * nvm_addr_read
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_read_naddrs_max(const struct nvm_dev *dev);

/**
 * Set the maximum number of addresses to use for writes, that is, when invoking
 * nvm_addr_write
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 */
int nvm_dev_get_write_naddrs_max(const struct nvm_dev *dev);

/**
 * Set the maximum number of addresses to use for erases, that is, when invoking
 * nvm_addr_erase
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param naddrs The maximum
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int nvm_dev_set_erase_naddrs_max(struct nvm_dev *dev, int naddrs);

/**
 * Sets whether retrieval and changes to bad-block-tables should be cached.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param bbts_cached 1 = cache enabled, 0 = cache disabled
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int nvm_dev_set_bbts_cached(struct nvm_dev *dev, int bbts_cached);

/**
 * Set the maximum number of addresses to use for erases, that is, when invoking
 * nvm_addr_erase.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param naddrs The maximum
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int nvm_dev_set_read_naddrs_max(struct nvm_dev *dev, int naddrs);

/**
 * Set the maximum number of addresses to use for erases, that is, when invoking
 * nvm_addr_erase.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param naddrs The maximum
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int nvm_dev_set_write_naddrs_max(struct nvm_dev *dev, int naddrs);

/**
 * Returns the geometry of the given device
 *
 * @note
 * See struct nvm_geo for the specifics of the returned geometry
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 *
 * @returns The geometry (struct nvm_geo) of given device handle
 */
const struct nvm_geo *nvm_dev_get_geo(const struct nvm_dev *dev);

/**
 * Allocate a buffer aligned to match the given geometry
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
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
 * Write content of buffer into file
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Destination where buffer will be dumped to
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int nvm_buf_to_file(char *buf, size_t nbytes, const char *path);

/**
 * Read content of file into buffer
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Source to read from
 *
 * @returns 0 on success, -1 on error and errno set to indicate the error.
 */
int nvm_buf_from_file(char *buf, size_t nbytes, const char *path);

/**
 * Erase nvm at given addresses
 *
 * @note
 * The addresses given to this function are interpreted as block addresses, in
 * contrast to `nvm_addr_mark`, `nvm_addr_write`, and `nvm_addr_read` for which
 * the address is interpreted as a sector address.
 *
 * @param dev Device handle obtained with `nvm_dev_open`
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
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addrs Array of memory address
 * @param naddrs Length of array of memory addresses
 * @param buf The buffer which content to write, must be aligned to device
 *            geometry of minimal write granularity and size equal to
 *            `naddrs * geo.nbytes`
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
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addrs List of memory address
 * @param naddrs Length of array of memory addresses
 * @param buf Buffer to store result of read into, must be aligned to device
 *            granularity min read and size equal to `naddrs *
 *            geo.sector_nbytes`
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
 * Checks whether the given address exceeds bounds of the given geometry
 *
 * @param addr The addr to check
 * @param geo The geometric bounds to check the given address against
 * @returns A mask of exceeded boundaries
 */
int nvm_addr_check(struct nvm_addr addr, const struct nvm_geo *geo);

/**
 * Converts a given physical address generic-format to device-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on generic-format to convert
 * @return Physical address on device-format
 */
uint64_t nvm_addr_gen2dev(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Converts a given physical address on device-format to generic-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on device-format to convert
 * @return Physical address on generic-format
 */
struct nvm_addr nvm_addr_dev2gen(struct nvm_dev *dev, uint64_t addr);

/**
 * Converts a given physical address on device-format to lba-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on device-format to convert
 * @return Physical address on lba-format
 */
uint64_t nvm_addr_dev2lba(struct nvm_dev *dev, uint64_t addr);

/**
 * Converts a given physical address on device-format to lba-offset-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on device-format to convert
 * @return Physical address on lba-offset-format
 */
uint64_t nvm_addr_dev2off(struct nvm_dev *dev, uint64_t addr);

/**
 * Converts a given physical address on generic-format to byte offset
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on generic-format to convert
 * @return Logical address as byte offset
 */
uint64_t nvm_addr_gen2off(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Converts a given byte offset to physical address on generic-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param off Logical address as byte offset to convert
 * @return Physical address on generic-format
 */
struct nvm_addr nvm_addr_off2gen(struct nvm_dev *dev, uint64_t off);

/**
 * Converts a given physical address on generic-format to LBA offset
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addr The physical address on generic-format to convert
 * @return Logical address as LBA offset
 */
uint64_t nvm_addr_gen2lba(struct nvm_dev *dev, struct nvm_addr addr);

/**
 * Converts a given LBA offset to physical address on generic-format
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param off Logical address as LBA offset to convert
 * @return Physical address on generic-format
 */
struct nvm_addr nvm_addr_lba2gen(struct nvm_dev *dev, uint64_t off);

/**
 * Prints a humanly readable representation of the given address
 *
 * @param addr The address to print
 */
void nvm_addr_pr(struct nvm_addr addr);

/**
 * Prints a humanly readable representation of the given list of addresses
 */
void nvm_addr_prn(struct nvm_addr *addr, unsigned int naddrs);

/**
 * Allocate a virtual block, spanning a given set of physical blocks
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param addrs Set of block-addresses forming the virtual block
 * @param naddrs The number of addresses in the address-set
 *
 * @returns On success, an opaque pointer to the initialized virtual block is
 * returned. On error, NULL and `errno` set to indicate the error.
 */
struct nvm_vblk *nvm_vblk_alloc(struct nvm_dev *dev, struct nvm_addr addrs[],
				int naddrs);

/**
 * Allocate a virtual block (spanning planes, channels, and LUNs)
 *
 * @param dev Device handle obtained with `nvm_dev_open`
 * @param ch_bgn Beginning of the channel span, as inclusive index
 * @param ch_end End of the channel span, as inclusive index
 * @param lun_bgn Beginning of the LUN span, as inclusive index
 * @param lun_end End of the LUN span, as inclusive index
 * @param blk Block index
 *
 * @returns On success, an opaque pointer to the initialized virtual block is
 * returned.  On error, NULL and `errno` set to indicate the error.
 */
struct nvm_vblk *nvm_vblk_alloc_line(struct nvm_dev *dev, int ch_bgn,
				     int ch_end, int lun_bgn, int lun_end,
				     int blk);

/**
 * Destroy a virtual block
 *
 * @param vblk The virtual block to destroy
 */
void nvm_vblk_free(struct nvm_vblk *vblk);

/**
 * Erase a virtual block
 *
 * @note
 * Erasing a vblk will reset internal position pointers
 *
 * @param vblk The virtual block to erase
 * @returns On success, the number of bytes erased is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_vblk_erase(struct nvm_vblk *vblk);

/**
 * Write to a virtual block
 *
 * @note
 * buf must be aligned to device geometry, see struct nvm_geo and nvm_buf_alloc
 * count must be a multiple of min-size, see struct nvm_geo
 * do not mix use of nvm_vblk_pwrite with nvm_vblk_write on the same virtual
 * block
 *
 * @param vblk The virtual block to write to
 * @param buf Write content starting at buf
 * @param count The number of bytes to write
 * @returns On success, the number of bytes written is returned and vblk
 * internal position is updated. On error, -1 is returned and `errno` set to
 * indicate the error.
 */
ssize_t nvm_vblk_write(struct nvm_vblk *vblk, const void *buf, size_t count);

/**
 * Write to a virtual block at a given offset
 *
 * @note
 * buf must be aligned to device geometry, see struct nvm_geo and nvm_buf_alloc
 * count must be a multiple of min-size, see struct nvm_geo
 * offset must be a multiple of min-size, see struct nvm_geo
 * do not mix use of nvm_vblk_pwrite with nvm_vblk_write on the same virtual
 * block
 *
 * @param vblk The virtual block to write to
 * @param buf Write content starting at buf
 * @param count The number of bytes to write
 * @param offset Start writing offset bytes within virtual block
 * @returns On success, the number of bytes written is returned. On error, -1 is
 * returned and `errno` set to indicate the error.
 */
ssize_t nvm_vblk_pwrite(struct nvm_vblk *vblk, const void *buf, size_t count,
			size_t offset);

/**
 * Pad the virtual block with synthetic data
 *
 * @note
 * Assumes that you have used nvm_vblk_write and now want to fill the remaining
 * virtual block in order to meet block write-before-read constraints
 *
 * @param vblk The virtual block to pad
 * @returns On success, the number of bytes padded is returned and internal
 * position is updated. On error, -1 is returned and `errno` set to indicate the
 * error.
 */
ssize_t nvm_vblk_pad(struct nvm_vblk *vblk);

/**
 * Read from a virtual block
 */
ssize_t nvm_vblk_read(struct nvm_vblk *vblk, void *buf, size_t count);

/**
 * Read from a virtual block at given offset
 */
ssize_t nvm_vblk_pread(struct nvm_vblk *vblk, void *buf, size_t count,
		       size_t offset);

/**
 * Retrieve the device associated with the given virtual block
 *
 * @param vblk The entity to retrieve information from
 */
struct nvm_dev *nvm_vblk_get_dev(struct nvm_vblk *vblk);

/**
 * Retrieve the set of addresses defining the virtual block
 *
 * @param vblk The entity to retrieve information from
 */
struct nvm_addr *nvm_vblk_get_addrs(struct nvm_vblk *vblk);

/**
 * Retrieve the number of addresses in the address set of the virtual block
 *
 * @param vblk The entity to retrieve information from
 */
int nvm_vblk_get_naddrs(struct nvm_vblk *vblk);

/**
 * Retrieve the size, in bytes, of a given virtual block
 *
 * @param vblk The entity to retrieve information from
 */
size_t nvm_vblk_get_nbytes(struct nvm_vblk *vblk);

/**
 * Retrieve the current cursor position for read to the virtual block
 *
 * @param vblk The entity to retrieve information from
 */
size_t nvm_vblk_get_pos_read(struct nvm_vblk *vblk);

/**
 * Retrieve the current cursor position for writes to the virtual block
 *
 * @param vblk The entity to retrieve information from
 */
size_t nvm_vblk_get_pos_write(struct nvm_vblk *vblk);

/**
 * Print the virtual block in a humanly readable form
 *
 * @param vblk The entity to print information about
 */
void nvm_vblk_pr(struct nvm_vblk *vblk);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLIGHTNVM.H */
