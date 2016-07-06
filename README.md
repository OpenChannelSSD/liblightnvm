# liblightnvm - Userspace I/O library for LightNVM

[![Build Status](https://travis-ci.org/safl/liblightnvm.svg?branch=master)](https://travis-ci.org/safl/liblightnvm)

liblightnvm is a user space library that manages provisioning of and I/O
submission to physical flash. The motivation is to enable I/O-intensive
applications to implement their own Flash Translation Tables (FTLs) directly on
their logic.  Our system design is based on the principle that high-performance
I/O applications use log-structured data structures to map flash constrains and
implement their own data placement, garbage collection, and I/O scheduling
strategies.  Using liblightnvm, applications have direct access to a physical
flash provisioning interface, and perceive storage as large single address
space flash pool from which they can allocate flash blocks from individual NAND
flash chips (LUNs) in the SSDs. This is relevant to match parallelism in the
host and in the device. Each application submits I/Os using physical addressing
that corresponds to specific block, page, etc. on device.

liblightnvm relies on Open-Channel SSDs, devices that share responsibilities
with the host to enable full host-based FTLs; and LightNVM, the Linux Kernel
subsystem supporting them. To know more about Open-Channel SSDs and the
software ecosystem around them please see [6].

For example, most key-value stores use Log Structured Merge Trees (LSMs) as
their base data structure (e.g., RocksDB, MongoDG, Apache Cassandra). The LSM
is in itself a form of FTL, which manages data placement and garbage
collection.  This class of applications can benefit from a direct path to
physical flash to take full advantage of the optimizations they do and spend
host resources on, instead of missing them through the several levels of
indirection that the traditional I/O stack imposes to enable genericity: page
cache, VFS, file system, and device physical - logical translation table.
liblightnvm exposes append-only primitives using direct physical flash to
support this class of applications.

# API

Liblightnvm allows direct usage of physical flash blocks. This means that the
library user is responsible for managing flash constraints such as writing
sequentially within a flash block, writing/erasing at flash page granularity,
reading at a sector granularity, etc.

Liblightnvm also provides a beam abstraction, encapsulating some of these
responsibilities, see section X.

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

The `get_blk` / `put_blk` interface allows the library user to obtain and yield
ownership of physical blocks on LightNVM managed media.

```c
/*
  Get a flash block from target *tgt* and LUN *lun* through the NVM_VBLOCK
  communication interface *prov*.
  returns On success, a flash block is allocated in LightNVM's media manager
  and *vblock* is filled up accordingly. If NVM_PROV_LUN_STATE is set,
  *lun_status* is also filled with LUN status metadata.  On error, -1 is
  returned, in which case *errno* is set to indicate the error.
*/
int nvm_vblock_get(int tgt, uint32_t lun, NVM_VBLOCK *vblock);
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
int nvm_vblock_put(int tgt, NVM_VBLOCK *vblock);
```

### I/O Submission

```c
/*
 * Write count number of flash pages of size fpage_size to the device.
 * fpage_size is the write page size. This is, the size of a virtual flash
 * pages, i.e., across flash planes.
 */
int nvm_vblock_write(int tgt, NVM_VBLOCK *vblock, const void *buf,
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
int nvm_vblock_read(int tgt, NVM_VBLOCK *vblock, void *buf, size_t ppa_off,
			size_t count, NVM_FLASH_PAGE *fpage, int flags);
```

## The beam abstraction / Append Only Interface

Build on top of the RAW I/O 

liblightnvm's I/O functionality is based around the concept of **beams**, i.e.,
I/O flows that operate directly with physical addresses and belong to a single
application, which is completely in control over data placement and garbage
collection. In the case of the append-only functionality, liblightnvm deals with
flash blocks internally and guarantees that a writer can append to an I/O beam
as long as there are available free blocks in the target and lun blocks are
being allocated from.

An application can *read* from a beam by providing an offset, without caring
about the actual flash blocks containing user data. A *sync* operator is also
provider to allow applications define barriers and persist data as needed. Flash
pages are padded when necessary to respect flash constrains; space amplification
must be accounted for when making use of the *sync* operator.

### Interface initialization and teardown

Initializes locks used by the beam abstraction. Should probably be removed such
that the library could be made re-entrant.

```c
/*
 * TODO: Describe
 */
int nvm_beam_init();
```

```c
/*
 * TODO: Describe
 */
void nvm_beam_exit();
```

### Beam Handle

```c
/*
 * Creates a new I/O beam associated with the target(tgt) in LUN(lun).
 * Flags:
 * NVM_PROV_SPEC_LUN - A LUN is specified. If there are free blocks in LUN
 *                     a block from that LUN is allocated.
 *                     Error is returned otherwise.
 * NVM_PROV_RAND_LUN - LightNVM's media manager selects the LUN from which
 *                     the block is allocated. If there are free blocks
 *                     available in tgt a block is allocated.
 *                     Error is returned otherwise.
 *
 * Returns: On success, a beam id that can be used to issue I/Os (e.g., append,
 * read). On error, -1 is returned, in which case *errno* is set to indicate the
 * error.
 */
int nvm_beam_create(const char *tgt, int lun, int flags);
```

```c
/*
 * Destroys the I/O beam associated with the id beam
 */
void nvm_beam_destroy(int _beam_, int _flags_);
```

### Beam I/O

```c
/*
 * Appends count bytes from buf to the I/O beam associated with the id beam.
 *
 * Returns: On success the number of bytes written to the beam is returned (zero
 * indicates that nothing has been written). On error, -1 is returned, in which
 * case errno is set to indicate the error.
 */
ssize_t nvm_beam_append(int _beam_, const void _*buf_, size_t _count_);
```

```c
/*
 * Reads count bytes to buffer buf from the I/O beam associated with the id
 * beam at offset.
 *
 * Returns: On success the number of bytes read from the beam is returned (zero
 * indicates that nothing has been read). On error, -1 is returned, in which case
 * errno is set to indicate the error.
 */
ssize_t nvm_beam_read(int beam, void *buf, size_t count, off_t offset,
                      int flags);
```

### Beam Synchronization

```c
/*
 * Syncs all data buffered on I/O beam *beam* to the device.
 *
 * Returns: On success, 0 is returned. On error, -1 is returned, in which case
 * errno is set to indicate the error.
 */
int nvm_beam_sync(int beam, int flags);
```

# Usage

WIP: This needs updating.

## Prerequisites


liblightnvm runs on top of LightNVM. However, support for it in the kernel is
not yet upstream. Development is taking place on the liblnvm branch [1]; please
check it out to test liblightnvm. This branch will follow master. This is, it
will have all new functionality and fixes sent upstream to LightNVM.

Also, it is necessary to have an Open-Channel SSD that supports LightNVM
commands. For testing purposes you can use QEMU. Please checkout [2] to get
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

### Retrieving information

... WIP ...

# Contact and Contributions

liblightnvm is in active development on the master branch. We will start using
feature branches when the library stabilizes. Also, the API is subject to
change, at least until kernel support has been sent upstream.

Please write to Javier at jg@lightnvm.io for more information.

# References

[1]: https://github.com/OpenChannelSSD/linux/tree/liblnvm "bla"
[2]: http://openchannelssd.readthedocs.org/en/latest/gettingstarted/#configure-qemu
[3]: https://github.com/OpenChannelSSD/qemu-nvme
[4]: https://github.com/OpenChannelSSD/lightnvm-hw
[5]: https://github.com/OpenChannelSSD/rocksdb
[6]: http://openchannelssd.readthedocs.org/en/latest/
