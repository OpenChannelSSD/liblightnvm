Vectorized IO to NAND media
===========================

With the basics of obtaining device information and constructing addresses in
place, one can dive into the task of creating commands for doing vectorized IO.

As the section on background information describes, then there are a handful of
constraints to handle for IOs to NAND media to succeed.

Erase before write
------------------

The first constraint to handle is that a **chunk** must be **erased** before it
can be **written**. We do so by constructing a **chunk-address**:

.. literalinclude:: nvm_cmd_vio_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_cmd_vio_00.out
   :language: bash

With the address we can now construct the erase command:

.. literalinclude:: nvm_cmd_vio_01.cmd
   :language: bash

On success, yielding:

.. literalinclude:: nvm_cmd_vio_01.out
   :language: bash

On error, yielding::

    # nvm_cmd_erase: {pmode: SNGL}
    naddrs: 1
    addrs:
      - {val: 0x0000001400000000, pugrp: 00, punit: 00, chunk: 0020, sectr: 0000}
    nvm_addr_erase: Input/output error
    nvm_ret { result(0xff), status(3) }

If an erase fails, it is most likely due to bad media. This is where the
information retrieved with ``nvm_cmd rprt`` is useful, that is, to avoid using
known bad chunks.

.. NOTE:: C API for performing erases using chunk-adressing is done with ``nvm_cmd_erase``

Write
-----

The two primary constraints for issuing writes are that they must be contiguous
within a chunk and span `ws_min` sectors.

.. NOTE :: C API for performing write using vectorized IO with addressing at
    sector-level is done using ``nvm_cmd_write``, note that the payload must
    be aligned to sector size, the helper function ``nvm_buf_alloc`` is
    provided for convenience

Minimum Write
~~~~~~~~~~~~~

For the geometry in figure X, the ``ws_min`` dictates that 24 sectors must be
written of each 4096 bytes, a command satisfying the minimum-write constraint
thus contains 24 addresses in sequential with a payload of 96Kbytes.

The command can be constructed via the CLI as:

.. literalinclude:: nvm_cmd_vio_02.cmd
   :language: bash

The CLI creates an arbitrary payload, so we do not concern us with the payload
at this point. The result of the command is:

.. literalinclude:: nvm_cmd_vio_02.out
   :language: bash

Optimal Write
~~~~~~~~~~~~~

For this specific device then ``ws_min`` is the same as ``ws_min``. However, an
improvement of round-trip-time can be obtained by increasing the amount of work
done by a single command, that is, increase the number of addresses to a
multiple of ``ws_opt`` and less than or equal to  write-naddrs-max.  In our
case we can construct a command with 48 addresses.

Abiding to all of the above mentioned constraints a write command can be
constructed as:

.. literalinclude:: nvm_cmd_vio_04.cmd
   :language: bash

Successfully yielding:

.. literalinclude:: nvm_cmd_vio_04.out
   :language: bash

Using vectorized IO we have with a single command successfully written a
payload of 48 x 4096 bytes = 192 KB.

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

.. literalinclude:: nvm_cmd_vio_05.cmd
   :language: bash

.. literalinclude:: nvm_cmd_vio_05.out
   :language: bash

What these two commands actually do will be described in the following section
on virtual blocks. For now all we need to know is that the block is now fully
written / closed and we can start reading from it.

.. NOTE :: C API for performing write using vectorized IO with addressing at
  sector-level is done using ``nvm_cmd_read``, the received payload must be
  stored in a sector-aligned buffer, the helper function ``nvm_buf_alloc`` is
  provided for convenience

Minimal Read
~~~~~~~~~~~~

We can read a single sector:

.. literalinclude:: nvm_cmd_vio_06.cmd
   :language: bash

.. literalinclude:: nvm_cmd_vio_06.out
   :language: bash

The data read from device can be written to a file system using the ``-o FILE``
option:

.. literalinclude:: nvm_cmd_vio_07.cmd
   :language: bash

.. literalinclude:: nvm_cmd_vio_07.out
   :language: bash

The payload is then available by inspection, e.g. with ``hexdump``:

.. literalinclude:: nvm_cmd_vio_08.cmd
   :language: bash

.. literalinclude:: nvm_cmd_vio_08.out
   :language: bash

Maximum Read
~~~~~~~~~~~~

Same as a write, a read has an upper bound on the number of addresses in a
single command. However, since there is no ``ws_min`` to align to, then the
read can utilize all ``NVM_NADDR_MAX``, that is 64 whereas the write was
confined to the even multiple less than or equal to ``NVM_NADDRS_MAX``, that
was, 48.

.. literalinclude:: nvm_cmd_vio_09.cmd
   :language: bash

Successfully yielding:

.. literalinclude:: nvm_cmd_vio_09.out
   :language: bash

Non-Contiguous Read
~~~~~~~~~~~~~~~~~~~

Reading 64 different sectors in arbitrary order:

.. literalinclude:: nvm_cmd_vio_10.cmd
   :language: bash

Successfully yielding:

.. literalinclude:: nvm_cmd_vio_10.out
   :language: bash

It is worth mentioning that vectorized reads can be non-contiguous not only
within a chunk but also scattered across chunks, in different PUNIT, and PUGRP.
