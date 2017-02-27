Virtual Block
=============

The physical addressing interface ``nvm_addr`` provides full control over the
construction of vectorized IO commands. It is known that with great power comes
great responsibility. Responsibility which increase the cognitive load on the
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

.. NOTE :: C API documentation of the virtual block is available in :ref:`sec-capi-vblk`

.. _sec-tut-vblk-plane:

Plane Span
----------

A virtual block will at a minimum consist of the given block address across all
planes. So it by default encapsulates concerns regarding plane-mode
constraints.

We erase, write, and read the virtual block:

.. literalinclude:: nvm_vblk_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_vblk_00.out
   :language: bash

.. _sec-tut-vblk-set:

Block Set
---------

To obtain parallel IO, in addition to plane-mode, we construct a virtual block
consisting of multiple physical blocks on distinct parallel units (LUNs)::

	(0x0000000000000002){ ch(00), lun(00), pl(0), blk(0002), pg(000), sec(0) }
	(0x0a0700000000014d){ ch(10), lun(07), pl(0), blk(0333), pg(000), sec(0) }
	(0x0301000000000014){ ch(03), lun(01), pl(0), blk(0020), pg(000), sec(0) }
	(0x0500000000000190){ ch(05), lun(00), pl(0), blk(0400), pg(000), sec(0) }

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

Block Line
----------

The block line provides convenient construction of a virtual block spanning
**lines** of physical blocks. With a block line, the block index is fixed and
one can specify true subsets of the parallel units.
For example, block 10 on all parallel units on the given device, which in this
case has 16 channels with each eight LUNs:

.. literalinclude:: nvm_vblk_line_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_vblk_line_00.out
   :language: bash
   :lines: 1-6

.. code-block:: none

   ... block address output omitted for brevity ...

.. literalinclude:: nvm_vblk_line_00.out
   :language: bash
   :lines: 133-136

.. literalinclude:: nvm_vblk_line_00.out
   :language: bash
   :lines: 137-142

.. code-block:: none

   ... block address output omitted for brevity ...

.. literalinclude:: nvm_vblk_line_00.out
   :language: bash
   :lines: 269-272

.. literalinclude:: nvm_vblk_line_00.out
   :language: bash
   :lines: 274-279

.. code-block:: none

   ... block address output omitted for brevity ...

.. literalinclude:: nvm_vblk_line_00.out
   :language: bash
   :lines: 406-409
