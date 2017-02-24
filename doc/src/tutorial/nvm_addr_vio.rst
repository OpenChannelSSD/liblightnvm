Vectorized IO to NAND media
===========================

With the basics of obtaining device information and constructing addresses in
place one can dive into the task of constructing commands for doing vectorized
IO.

As the section on background information describes, then there are handful of
constraints to handle for IOs to NAND media to succeed.

Erase before write
------------------

The first constraint to handle is that a block must be **erased** before it can
be **written**. We do so by constructing block-addresses for all blocks within
a plane:

.. literalinclude:: nvm_addr_vio_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_addr_vio_00.out
   :language: bash

With the addresses we can now construct a single command with two addresses and
send of the erase:

.. literalinclude:: nvm_addr_vio_01.cmd
   :language: bash

On success, yielding:

.. literalinclude:: nvm_addr_vio_01.out
   :language: bash

On error, yielding::

    ** nvm_addr_erase(...) : pmode(0x1)
    (0x000000000000000a){ ch(00), lun(00), pl(0), blk(0010), pg(000), sec(0) }
    (0x000001000000000a){ ch(00), lun(00), pl(1), blk(0010), pg(000), sec(0) }
    nvm_addr_erase: Input/output error
    nvm_ret { result(0xff), status(3) }

If an erase fails as above then it is because a block is bad. The
bad-block-table interface (nvm_bbt) is provided to communicate and update media
state. Introduction to the bad-block-table is given in a later section.

.. NOTE:: C API for performing erases using block-adressing is done with ``nvm_addr_erase``

Write
-----

The two primary constraints for issuing writes are that they must be at the
granularity of a full flash page and contiguous within a block.

.. NOTE :: C API for performing write using vectorized IO with addressing at
    sector-level is done using ``nvm_addr_write``, note that the payload must
    be aligned to sector size, the helper function ``nvm_buf_alloc`` is
    provided for convenience

Minimum write
~~~~~~~~~~~~~

For the geometry in figure X, a full flash page is four sectors of each 4096
bytes, a command satisfying the minimum-write constraint thus contains four
addresses with a payload of 16384 bytes of data. The command can constructed
via the CLI as:

.. literalinclude:: nvm_addr_vio_02.cmd
   :language: bash

The CLI creates an arbitrary payload, so we do not concern us with the
payload at this point.

The result of the command is:

.. literalinclude:: nvm_addr_vio_02.out
   :language: bash

An unexpected write error occured, the constraints are satisfied, so what goes
wrong? The issue in this case is that the command is constructed using
plane-mode.

This introduces an additional constraint that writes must be performed to the
block accross all planes. One can choose to disable the plane-mode, which is
done by setting environment var ``NVM_CLI_PMODE="0"`` or by constructing a
command satisfying the plane-mode constraint:

.. literalinclude:: nvm_addr_vio_03.cmd
   :language: bash

Yielding without error:

.. literalinclude:: nvm_addr_vio_03.out
   :language: bash

The plane-mode allows for multiple writes to be done in parallel across the
planes.

Maximum write
~~~~~~~~~~~~~

An improvement of round-trip-time can be obtained by increasing the amount of
work done by a single command, that is, increase the number of addresses.

There is an upper limit, write-naddrs-max,  as can be seen in figure X, and
retrieved from the device. In our case we can construct a command with 64
addresses.

Abiding to all of the above mentioned constraints a write command can be
constructed as:

.. literalinclude:: nvm_addr_vio_04.cmd
   :language: bash

Successfully yielding:

.. literalinclude:: nvm_addr_vio_04.out
   :language: bash

Using vectorized IO we have with a single command successfully written a
payload of 64 x 4096 bytes = 256 KB.

Read
----

Reads have fewer constraints than writes. The granularity of a read is a single
sector (the smallest addressable unit) and can be performed non-contiguously.

The primary constraint for a read to adhere to is that the block which is read
from must be closed. That is, all pages within the block must have been
written. It might be that the constraint can be relaxed where only N pages
ahead of the read must have been written instead of all pages in the block. The
challenge with relaxing the constraint is that N is often an unknown size.

We have so far written a total of nine pages (across two planes), the first
page in one command, the remaining eight pages in a second command. Thus we
have 503 pages that need to be written before we can start reading.

Specifying the 503 x nplanes x nsectors = 4024 addresses via the CLI is
tedious, we will therefore take a sneak peak at virtual blocks and execute:

.. literalinclude:: nvm_addr_vio_05.cmd
   :language: bash

.. literalinclude:: nvm_addr_vio_05.out
   :language: bash

What these two commands actually do will be described in the following section
on virtual blocks. For now all we need to know is that the block is now fully
written / closed and we can start reading from it.

.. NOTE :: C API for performing write using vectorized IO with addressing at
  sector-level is done using ``nvm_addr_read``, the received payload must be
  stored in a sector-aligned buffer, the helper function ``nvm_buf_alloc`` is
  provided for convenience

Minimal Read
~~~~~~~~~~~~

We can read a single sector:

.. literalinclude:: nvm_addr_vio_06.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_addr_vio_06.out
   :language: bash

Setting the environment var ``NVM_CLI_BUF_PR``:

.. literalinclude:: nvm_addr_vio_07.cmd
   :language: bash

Will dump the read payload to stdout:

.. literalinclude:: nvm_addr_vio_07.out
   :language: bash


Maximum Read
~~~~~~~~~~~~

Same as a write, a read has an upper bound on the number of addresses in a
single command. Address-construction is equivalent.

Non-Contiguous Read
~~~~~~~~~~~~~~~~~~~

Reading pages 500, 200, 0, and 6 across planes:

.. literalinclude:: nvm_addr_vio_08.cmd
   :language: bash

Successfully yielding:

.. literalinclude:: nvm_addr_vio_08.out
   :language: bash

It is worth mentioning that the vectorized reads can be non-contiguous not only
within a block but also scattered across different different blocks, in
different LUNs, and channels.

However, when using plane-mode ensure that addresses are constructed across
planes and all sectors are read as in the example above.

