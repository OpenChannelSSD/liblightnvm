.. _sec-background:

============
 Background
============

Dealing directly with physical media requires understanding its characteristics
and respecting its constraints. In this section, we will discuss general
constraints, good manners, and rules of thumb specific to NAND flash memories.
However, since all NAND is slightly different, some NAND might have extra
constraints or optimizations not described here and only available when working
directly with vendors.

Device Architecture
===================

NAND flash provides a read- /write/erase interface. Within a NAND package,
storage media is organized into a hierarchy of die, plane, block, and page. A
die allows a single I/O command to be executed at a time. There may be one or
several dies within a single physical package. A plane allows similar flash
commands to be executed in parallel within a die. The die defines then the
parallel unit of the device.

Within each plane, NAND is organized in blocks and pages. Each plane contains
the same number of blocks, and each block contains the same number of pages.
Pages are the minimal units of read and write, while the unit of erase is a
block. Each page is further decomposed into fixed-size sectors with an
additional out-of-bound area, e.g., a 16KB page contains four sectors of 4KB
and out-of-bound area frequently used for ECC and user-specific data.

Media Constraints
=================

Writes
------

There are three fundamental programming constraints that apply to NAND:

1. A write command must always contain enough data to program one (or several)
   full flash page(s)
2. A write must be sequential within a block
3. An erase must be performed before a page within a block can be (re)written.
   The number of program/erase (PE) cycles is limited. The limit depends on the
   type of flash: 10E2 for TLC/QLC flash, 10E3 for MLC, or 10E5 for SLC.

Additional constraints must be considered for different types of NAND flash.
For example, in multi-level cell memories, the bits stored in the same cell
belongs to different write pages, referred to as lower/upper pages. The upper
page must be written before the lower page can be read successfully. The lower
and upper page are often not sequential, and any pages in between must be
written to prevent write neighbor disturbance. Not writing all lower/upper
pages before reading a sector might result on a read error. This is discussed
in the next section.

The size of a flash page when writing, must be calculated considering the media
access mode, i.e., the number of planes being used. For example, a NAND flash
with 4KB sectors, 4 sectors per page and operating in dual-plane mode, must
write 32KB of data (4KB * 4 sectors * 2 planes) before a "flash page" is
written. In this case, hints might be used in the write command to inform the
hardware that a dual-plane write has been issue. This allows to process the
writes in parallel within the controller.

Reads
-----

Reads can be done at the granularity of a single sector, which is typically
4KB.  Lower granularities must be handled internally by liblightnvm clients.

Typically, reads issued to a closed block, i.e., a block that is fully written,
will succeed. Reads issued to an open block must respect that all lower/upper
pages within the pages being read has been fully written. Note that lower/upper
pages might be shared across flash pages. Since the actual sequence for
lower/upper pages is usually protected under NDA, it might be necessary to
consult the hardware manufacturer for information about the specifics of the
used media. A rule of thumb is to write 8-12 pages after a page can be read.
The data in between must be cached by the client if it is required to be read
within this threshold.

Failure Modes
-------------

Failure Modes. NAND Flash might fail in various ways:

Bit Errors.
  The downside of shrinking cell size is an increase in errors when storing
  bits. While error rates of 2 bits per KB were common for SLC, this rate has
  increased four to eight times for MLC.

Read and Write Disturb.
  The media is prone to leak currents to nearby cells as bits are written or
  read. This causes some of the write constraints described above.

Data Retention.
  As cells wear out, data retention capability decreases. To persist over time,
  data must be rewritten multiple times.

Write/Erase Error.
  During write or erase, a failure can occur due to an unrecoverable error at
  the block level. In that case, the block should be retired and data already
  written should be rewritten to a new block.

Die Failure.
  A logical unit of storage, i.e., a die on a NAND chip may cease to function
  over time due to a defect. In that case, all its data will be lost unless
  RAID or other replication techniques has been used.

REFERENCES
==========

  * LightNVM: the Linux Open-Channel SSD Subsystem" - Matias Bjørling, Javier Gonzalez, Philippe Bonnet - FAST 2017.
