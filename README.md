# liblightnvm - Userspace I/O library for LightNVM

[![Build Status](https://travis-ci.org/OpenChannelSSD/liblightnvm.svg?branch=master)](https://travis-ci.org/OpenChannelSSD/liblightnvm)
[![Download tgz](https://api.bintray.com/packages/openchannelssd/binaries/liblightnvm/images/download.svg) ](https://bintray.com/openchannelssd/binaries/liblightnvm/_latestVersion)
[![Download deb](https://api.bintray.com/packages/openchannelssd/debs/liblightnvm/images/download.svg) ](https://bintray.com/openchannelssd/debs/liblightnvm/_latestVersion)

liblightnvm is a user space library that manages provisioning of and I/O
submission to physical flash. The motivation is to enable I/O-intensive
applications to implement their own Flash Translation Layer (FTLs) using the
internal application data structures.  The design is based on the principle that
high-performance I/O applications often use structures that assimilates
structures found within a Flash translation layer. This include log-structured
data structures that provides their own mechanisms for data placement, garbage
collection, and I/O scheduling strategies.  

For example, popular key-value stores often use a form of Log Structured Merge
Trees (LSMs) as their base data structure (including RocksDB, MongoDB, Apache
Cassandra). The LSM is in itself a form of FTL, which manages data placement and
garbage collection.  This class of applications can benefit from a direct path
to physical flash to take full advantage of the optimizations they do and spend
host resources on, instead of missing them through the several levels of
indirection that the traditional I/O stack imposes to enable genericity: page
cache, VFS, file system, and device physical - logical translation table.
liblightnvm exposes append-only primitives using direct physical flash to
support this class of applications.

# API

liblightnvm allows direct usage of physical flash blocks. This means that the
library user is responsible for managing flash constraints such as writing
sequentially within a flash block, writing/erasing at flash page granularity,
reading at a sector granularity, etc.

Data structures for representing luns, blocks, and pages are described in the
following section. Followed by a description the functions for provisioning
blocks and submitting I/Os.

Note that standard integer types are used to match the user space types defined
in the LightNVM kernel API.

## Data structures

A **NVM_VBLOCK** represents a virtual flash block. We refer to it as "virtual"
since it can be formed by a collection of physical blocks stripped across flash
planes and/or LUNs, as defined by the controller on the Open-Channel SSD.

NVM_VBLOCK is the unit at which LightNVM's media manager provisioning interface
operates.

```c
NVM_VBLOCK {
	__uint64_t id;
	__uint64_t bppa;
	__uint32_t vlun_id;
	__uint32_t owner_id;
	__uint32_t nppas;
	__uint16_t ppa_bitmap;
	__uint16_t flags;
};
```

**NVM_FLASH_PAGE** TODO: describe

```c
NVM_FLASH_PAGE {
	uint32_t sec_size;
	uint32_t page_size;
	uint32_t pln_pg_size;
	uint32_t max_sec_io;
};
```

## Functions


### Target Handle

```c
/*
  Open target tgt.

  returns On success, a file descriptor to the target is returned. This file
  descriptor is the one provided by the nvm_get_block / nvm_put_block interface.
  By using nvm_target_open instead of directly opening a file descriptor to the
  target path, liblightnvm can keep track of which targets are being used and
  manage memory accordingly. On error, -1 ie returned, in which case *errno* is
  set to indicate the error.
*/
int nvm_tgt_open(const char *tgt, int flags);
```

```c
/*
  Close target tgt.
*/
void nvm_tgt_close(int tgt);
```

### Provisioning

The `nvm_vblock_get` / `nvm_vblock_put` interface allows the library user to
obtain and yield ownership of physical blocks on LightNVM managed media.

```c
/**
 * Reserves a block on given target using a specific lun
 *
 * @param vblock Block created with nvm_vblock_new
 * @param tgt Handle obtained with nvm_tgt_open
 * @param lun Identifier of lun to reserve via
 * @return -1 on error and errno set, zero otherwise.
 */
int nvm_vblock_gets(NVM_VBLOCK *vblock, NVM_TGT tgt, uint32_t lun);

/**
 * Reserve a block on given target using an arbitrary lun
 *
 * @param vblock Block created with nvm_vblock_new
 * @param tgt Handle obtained with nvm_tgt_open
 * @param lun Identifier of lun to reserve via
 * @return -1 on error and errno set, zero otherwise.
 */
int nvm_vblock_get(NVM_VBLOCK vblock, NVM_TGT tgt);
```

```c
/*
  Put flash block *vblock* back to the target *tgt*. This action implies that
  the owner of the flash block previous to this function call no longer owns the
  flash block, and therefor an no longer submit I/Os to it, or expect that data
  on it is persisted. The flash block cannot be reclaimed by the previous owner.

  returns  On success, a flash block is returned to LightNVM's media manager. If
  NVM_PROV_LUN_STATE is set, *lun_status* is also filled with LUN status
  metadata. On error, -1 is returned, in which case *errno* is set to indicate
  the error.
*/
int nvm_vblock_put(NVM_VBLOCK vblock, NVM_TGT tgt);
```

### I/O Submission

```c
/*
 * Write count number of flash pages of size fpage_size to the device.
 * fpage_size is the write page size. This is, the size of a virtual flash
 * pages, i.e., across flash planes.
 */
int nvm_vblock_pwrite(int tgt, NVM_VBLOCK *vblock, const void *buf,
			size_t ppa_off, size_t count,
			NVM_FLASH_PAGE *fpage, int flags);
```

```c
/*
 * Read count number of flash pages of size fpage_size from the device.
 * fpage_size is the read page size, which might be smaller than the write page
 * size; some controllers support reading at a sector granurality (typically
 * 4KB), instead of reading the whole virtual flash page (across planes).
 *
 * XXX(1): For now, we assume that the device supports reading at a sector
 * granurality; we will take this information from the device in the future.
 */
int nvm_vblock_pread(int tgt, NVM_VBLOCK *vblock, void *buf, size_t ppa_off,
			size_t count, NVM_FLASH_PAGE *fpage, int flags);
```

## Prerequisites


liblightnvm runs on top of LightNVM. However, support for it in the kernel is
not yet upstream. Development is taking place on the liblnvm branch [1]; please
check it out to test liblightnvm. This branch will follow master. This is, it
will have all new functionality and fixes sent upstream to LightNVM.

Also, it is necessary to have an Open-Channel SSD that supports LightNVM
command set. For testing purposes you can use QEMU. Please checkout [2] to get
Keith Busch's qemu-nvme with support for Open-Channel SSDs.
Documentation on how to setup LightNVM and QEMU can be found in [3].
Refer to [4] for LightNVM sanity checks.

Once LightNVM is running on QEMU you can install liblightnvm and test it. Note
that superuser privileges are required to install the library. Also, superuser
privileges are required to execute the library sanity checks. This is because
LightNVM target creation requires it.

To run sanity checks:

```bash
make dev test
```

Refer to examples/ and tests/ to find example code.

A storage backend for RocksDB using liblightnvm is under development [5].

To know more about Open-Channel SSDs and the software ecosystem around
them please see [6].

## Management

To obtain access to a storage device, the device must be LigthNVM enabled and a
LightNVM target created on top of the device. These tasks can be done with
nvme-cli[1], example follows.

Enable LightNVM on the device:
```
nvme lnvm init -d nvme0n1
```

Create a `dflash` target on the device:
```
nvme lnvm create --device-name nvme0n1 \
                 --target-name tgt0 \
                 --target-type dflash \
                 --lun-begin 0 \
                 --lun-end 63
```

With the above commands then the nvme device `nvme0n1` is LigthNVM enabled and
a `dflash` target named `tgt0` created capable of utilizing luns 0-63 of
`nvme0n1`.

### Retrieving geometry information

To be filled in

# Contact and Contributions

liblightnvm is in active development and pull requests are very welcome.

# NAND flash -- 101 -- bottom up

NAND is based on cells, typically called memory cells since these are the units
used to store information. The amount of information that can be stored in a
cell varies based on the NAND type.

Widely available NAND include SLC, MLC, and TLC. An SLC cell can be in two
states high or low, representing one bit of information. An MLC cell typically
has four states

Key characteristic is that each
type stores one, two or three bits of information per cell respectfully.

With higher density comes lower endurance.

When NAND flash is erased, gates are set high, when they are programmed they
are set low. Erasing a NAND flash block means setting all gates to high, the
amount of times this can be done varies based on the type of NAND media.
Here are some vague numbers.

Type - Erases per block

SLC | 100000
MLC | 1000 - 10.000
TLC | 1000

# References

   1. https://github.com/OpenChannelSSD/linux/tree/liblnvm 
   2. http://openchannelssd.readthedocs.org/en/latest/gettingstarted/#configure-qemu
   3. https://github.com/OpenChannelSSD/qemu-nvme
   4. https://github.com/OpenChannelSSD/lightnvm-hw
   5. https://github.com/OpenChannelSSD/rocksdb
   6. http://openchannelssd.readthedocs.org/en/latest/
