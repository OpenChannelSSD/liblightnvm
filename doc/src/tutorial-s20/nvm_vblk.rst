Virtual Block
=============

The command interface ``nvm_cmd`` provides full control over the construction
of vectorized IO commands. It is known that with great power comes great
responsibility. Responsibility which increase the cognitive load on the
developer when integrating vectorized IO at the application level.

liblightnvm, therefore, introduces a pure software abstraction, a virtual
block, to reduce the cognitive load for application developers.

A virtual block behaves as a physical, that is, the constraints of working with
NAND media also apply to a virtual block. However, the abstraction encapsulates
the command and address construction of parallel vectorized IO and exposes a
flat address space which is read/written in a manner equivalent to the
read/write primitives offered by libc.

The minimum construction encapsulates managing address-construction across
planes and setting plane hints. See section :ref:`sec-tut-vblk-plane`.
Increased utilization of parallel units in a device is achieved by constructing
a virtual block as either a :ref:`sec-tut-vblk-set` or a
:ref:`sec-tut-vblk-line`.

.. NOTE :: C API documentation of the virtual block is available in
  :ref:`sec-capi-nvm_vblk`

.. _sec-tut-vblk-plane:

Single Chunk
------------

A virtual block will at a minimum consist of a single chunk.

We erase, write, and read the virtual block:

.. literalinclude:: nvm_vblk_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_vblk_00.out
   :language: bash

.. _sec-tut-vblk-set:

Chunk Set
---------

To obtain parallel IO, we construct a virtual block
consisting of multiple chunks on distinct parallel units::

  {val: 0x0000008e00000000, pugrp: 00, punit: 00, chunk: 0142, sectr: 0000}
  {val: 0x0100008e00000000, pugrp: 01, punit: 00, chunk: 0142, sectr: 0000}
  {val: 0x0200008e00000000, pugrp: 02, punit: 00, chunk: 0142, sectr: 0000}
  {val: 0x0300008e00000000, pugrp: 03, punit: 00, chunk: 0142, sectr: 0000}

We erase, write and read the virtual block:

.. literalinclude:: nvm_vblk_01_set.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_vblk_01_set.out
   :language: bash

This demonstrates a weak-scaling experiment, increasing the workload
proportionally with the parallel units consumes approximately the same amount
of wall-clock time, thus achieving near linear speedup by utilizing parallel
units on the device.

.. NOTE :: The difference between the plane span and the block set is merely
  that the virtual block creation uses multiple block-addresses instead of one

.. _sec-tut-vblk-line:

Chunk Line
----------

The chunk line provides convenient construction of a virtual block spanning
**lines** of chunks. With a chunk line, the block index is fixed and one can
specify true subsets of the parallel units.

For example, chunk 142 on all parallel units on the given device, which in this
case has eight parallel unit groups, each with four parallel units:

.. literalinclude:: nvm_vblk_line_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_vblk_line_00.out
   :language: bash

