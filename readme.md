# liblightnvm - User Space Support for Application-Driven FTLs on Open-Channel SSDs

liblightnvm is a user space library that manages provisioning and I/O submission
for physical flash. The motivation is to enable I/O-intensive applications to
implement their own Flash Translation Tables (FTLs) directly on their logic.
Our system design is based on
the principle that high-performance I/O applications use log-structured data
structures to map flash constrains and implement their own data placement,
garbage collection,
and I/O scheduling strategies. Using liblightnvm, applications have direct
access to a physical flash provisioning interface, and perceive storage as large
single address space
flash pool from which they can allocate flash blocks from individual NAND flash
chips (LUNs) in the SSDs. This is relevant to match parallelism in the host and
in the device. Each application submits I/Os using physical addressing that
corresponds to specific block, page, etc. on device.

liblightnvm relies on Open-Channel SSDs, devices that share responsibilities
with the host to enable full host-based FTLs; and LightNVM, the Linux Kernel
subsystem supporting them. To know more about Open-Channel SSDs and the software
ecosystem around them please see [6].

For example, most key-value stores use Log Structured Merge Trees (LSMs) as
their base data structure (e.g., RocksDB, MongoDG, Apache Cassandra). The LSM is
in itself a form of FTL, which manages data placement and garbage collection.
This class of applications can benefit from a direct path to physical flash to
take full advantage of the optimizations they do and spend host resources on, instead of
missing them through the several levels of indirection that the traditional I/O
stack imposes to enable genericity: page cache, VFS, file system, and device
physical - logical translation table. liblightnvm exposes append-only primitives
using direct physical flash to support this class of applications.

## API description

liblightnvm's API is divided in two parts: (i) a management interface, and (ii)
an I/O interface. The raw I/O interface is a simple get_block / put_block API
that allows applications to directly deal with physical flash blocks. To
minimize changes on the application side, we also provide an append-only
interface,which targets applications that can handle data placement and garbage
collection in their primary data structures (e.g. Log-Structured Merge Trees).
As we target more classes of applications, the API will expand to target them.

### Management API

liblightnvm's management API enables applications to easily create and remove
LightNVM targets, and query device, media, and target information. The
structures sent in the ioctls used to communicate with LightNVM are directly
exposed in the API to enable flexibility.

- *int nvm_get_info(struct nvm_ioctl_info *);*
- *int nvm_get_devices(struct nvm_ioctl_get_devices *);*
- *int nvm_create_target(struct nvm_ioctl_tgt_create *);*
- *int nvm_remove_target(struct nvm_ioctl_tgt_remove *);*
- *int nvm_get_device_info(struct nvm_ioctl_dev_info *);*
- *int nvm_get_target_info(struct nvm_ioctl_tgt_info *);*

### Append-only API

liblightnvm's I/O functionality is based around the concept of **beams**,
i.e., I/O flows that operate directly with physical addresses and belong to a
single application, which is completely in control over data placement and
garbage collection. In the case of the append-only functionality, liblightnvm
deals with flash blocks internally and guarantees that a writer can append to an
I/O beam as long as there are available free blocks in the target and lun blocks
are being allocated from.

An application can *read* from a beam by providing an offset, without caring about
the actual flash blocks containing user data. A *sync* operator is also provider
to allow applications define barriers and persist data as needed. Flash pages
are padded when necessary to respect flash constrains; space amplification must
be accounted for when making use of the *sync* operator.

- **int nvm_beam_create(const char _*tgt_, int _lun_, int _flags_);**
  - Description:
  Creates a new I/O beam associated with the target *tgt* in LUN *lun* (see flags).
  - Return Value:
  On success, a beam id that can be used to issue I/Os (e.g., append, read). On
  error, -1 is returned, in which case *errno* is set to indicate the error.
  - Flags:
    - *NVM_PROV_SPEC_LUN* - A LUN is specified. If there are free blocks in LUN
      *lun* a block from that LUN is allocated. Error is return otherwise.
    - *NVM_PROV_RAND_LUN* - LightNVM's media manager selects the LUN from which
      the block is allocated. If there are free blocks available to the target
      *tgt* a block is allocated. Error is returned otherwise.

- **void nvm_beam_destroy(int _beam_, int _flags_);**
  - Description:
  Destroys the I/O beam associated with the id *beam*.

- **ssize_t nvm_beam_append(int _beam_, const void _*buf_, size_t _count_);**
  - Description:
  Appends *count* bytes from buffer *buf* to the I/O beam associated with the id
  *beam*.
  - Return Value:
  On success the number of bytes written to the beam is returned (zero indicates
  that nothing has been written). On error, -1 is returned, in which case
  *errno* is set to indicate the error.

- **ssize_t nvm_beam_read(int _beam_, void _*buf_, size_t _count_, off_t
  _offset_, int _flags_);**
  - Description:
  Reads *count* bytes to buffer *buf* from the I/O beam associated with the id
  *beam* at offset *offset*.
  - Return Value:
  On success the number of bytes read from the beam is returned (zero indicates
  that nothing has been read). On error, -1 is returned, in which case
  *errno* is set to indicate the error.

- **int nvm_beam_sync(int _beam_, int _flags_);**
  - Description:
  Syncs all data buffered on I/O beam *beam* to the device.
  - Return Value:
  On success, 0 is returned. On error, -1 is returned, in which case *errno* is
  set to indicate the error.

## Raw I/O API

TODO

## How to use

liblightnvm runs on top of LightNVM. However, the code is still not ready to
be sent upstream. Development is taking place on the liblnvm branch [1]; please
check it out to test liblightnvm. This branch will follow master. This is, it
will have all new functionality and fixes sent upstream to LightNVM.

Also, it is necessary to have an Open-Channel SSD that supports LightNVM
commands. For testing purposes you can use QEMU. Please checkout [2] to get
Keith Busch's qemu-nvme with support for Open-Channel SSDs. Documentation on how
to setup LightNVM and QEMU can be found in [3]. Refer to [4] for LightNVM sanity
checks.

Once LightNVM is running on QEMU you can install liblightnvm and test it. Note
that superuser privileges are required to install the library. Also, superuser
privileges are required to execute the library sanity checks. This is because
LightNVM target creation requires it.

- *$ sudo make install_local* - Copy the necessary headers
- *$ sudo make install* - Install liblightnvm
- *$ sudo make check* - Execute sanity checks

Refer to tests/ to find example code.

A storage backend for RocksDB using liblightnvm will follow soon [5]

To know more about Open-Channel SSDs and the software ecosystem around
them please see [6].

[1] https://github.com/OpenChannelSSD/linux/tree/liblnvm <br />
[2]
http://openchannelssd.readthedocs.org/en/latest/gettingstarted/#configure-qemu
<br />
[3] https://github.com/OpenChannelSSD/qemu-nvme <br />
[4] https://github.com/OpenChannelSSD/lightnvm-hw <br />
[5] https://github.com/OpenChannelSSD/rocksdb <br />
[6] http://openchannelssd.readthedocs.org/en/latest/ <br />

## Contact and Contributions

liblightnvm is in active development. Until the basic functionality is in
place, development will take place on master. We will start using feature
branches when the library reaches a stable version.

Please write to Javier at jg@lightnvm.io for more information.
