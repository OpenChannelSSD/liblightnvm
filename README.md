User space I/O library for Open-channel SSDs
============================================

**Status** [![Build Status](https://travis-ci.org/OpenChannelSSD/liblightnvm.svg?branch=master)](https://travis-ci.org/OpenChannelSSD/liblightnvm)
**docs** [http://lightnvm.io/liblightnvm](http://lightnvm.io/liblightnvm)

liblightnvm is a user space library that manages provisioning of and I/O
submission to physical flash. The motivation is to enable I/O-intensive
applications to implement their own Flash Translation Layer (FTLs) using the
internal application data structures. The design is based on the principle that
high-performance I/O applications often use structures that assimilates
structures found within a Flash translation layer.  This include log-structured
data structures that provides their own mechanisms for data placement, garbage
collection, and I/O scheduling strategies.

For example, popular key-value stores often use a form of Log Structured Merge
Trees (LSMs) as their base data structure (including RocksDB, MongoDB, Apache
Cassandra). The LSM is in itself a form of FTL, which manages data placement and
garbage collection. This class of applications can benefit from a direct path to
physical flash to take full advantage of the optimizations they do and spend
host resources on, instead of missing them through the several levels of
indirection that the traditional I/O stack imposes to enable genericity: page
cache, VFS, file system, and device physical - logical translation table.
liblightnvm exposes append-only primitives using direct physical flash to
support this class of applications.

Contact and Contributions
=========================

liblightnvm is in active development and maintained by Simon A. F. Lund
<slund@cnexlabs.com>, pull requests are very welcome.

References
==========

* [Open-channel SSDs](http://openchannelssd.readthedocs.org/en/latest/)
* [Quick Start](http://lightnvm.io/liblightnvm/quick_start/index.html)
* [nvme-cli](https://github.com/linux-nvme/nvme-cli)
* [qemu-nvme](https://github.com/OpenChannelSSD/qemu-nvme)
* [rocksdb support](https://github.com/OpenChannelSSD/rocksdb)

