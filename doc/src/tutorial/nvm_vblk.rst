Virtual Block
=============

The physical addressing interface ``nvm_addr`` provides full control over the
construction of vectorized IO commands. It is known that with great power comes
great responsibility, responsibility which increase the cognitive load on the
developer when integrating vectorized IO directly at the application level.

liblightnvm introduces a pure software abstraction, a virtual block, to reduce
the cognitive load for application developers.

A virtual block behaves as a physical, that is, the constraints of working with
NAND media also apply to a virtual block. However, the abstraction encapsulates
the command and address construction of parallel vectorized IO and exposes a
flat address space which is read/written in a manner equivalent to the
read/write primitives offered by libc.

A virtual block will at a minimum consist of the given block address across all
planes. So it by default encapsulates concerns regarding plane-mode
constraints.

We erase, write, and read the virtual block:

.. literalinclude:: nvm_vblk_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_vblk_00.out
   :language: bash

Virtual Block Set
-----------------

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

Virtual Block Line
------------------

