.. _sec-cli-vblk:

nvm_vblk
========

.. literalinclude:: nvm_vblk_usage.out
   :language: none

A virtual block consists, at a minimum, of the physical blocks at a given block
address across all planes in a LUN. The minimum construction encapsulates
managing address-construction across planes and setting plane hints. See
section :ref:`sec-cli-vblk-plane`.

Increased utilization of parallel units in a device is achieved by constructing
a virtual block as either a :ref:`sec-cli-vblk-set` or a
:ref:`sec-cli-vblk-line`.

The write operation by default uses a synthetically constructed payload, use
the ``-i FILE`` option to provide a payload from file. Payloads can likewise be
dumped to file system when read using the ``-o FILE`` option.

.. tip:: See section :ref:`sec-cli-env` for a full list of environment
  variables modifying command behavior

.. SEEALSO:: See :ref:`sec-cli-nvm_addr` on how to construct the ``0xADDR`` parameter

.. _sec-cli-vblk-plane:

Plane Span
----------

Construct a virtual block providing the address of block 42 in LUN 5 on channel
2:

.. literalinclude:: nvm_vblk_00_addr.cmd
   :language: bash

.. literalinclude:: nvm_vblk_00_addr.out
   :language: bash

Erase
~~~~~

.. literalinclude:: nvm_vblk_00_erase.cmd
   :language: bash

.. literalinclude:: nvm_vblk_00_erase.out
   :language: bash

Write
~~~~~

.. literalinclude:: nvm_vblk_01_write.cmd
   :language: bash

.. literalinclude:: nvm_vblk_01_write.out
   :language: bash

Read
~~~~

.. literalinclude:: nvm_vblk_02_read.cmd
   :language: bash

.. literalinclude:: nvm_vblk_02_read.out
   :language: bash

.. _sec-cli-vblk-set:

Block Set
---------

Construct a virtual block using an arbitrary set of physical block addresses.
E.g. construct a virtual block spanning the physical blocks::

  (0x0000000000000002){ ch(00), lun(00), pl(0), blk(0002), pg(000), sec(0) }
  (0x0a0700000000014d){ ch(10), lun(07), pl(0), blk(0333), pg(000), sec(0) }
  (0x0301000000000014){ ch(03), lun(01), pl(0), blk(0020), pg(000), sec(0) }
  (0x0500000000000190){ ch(05), lun(00), pl(0), blk(0400), pg(000), sec(0) }

Erase
~~~~~

.. literalinclude:: nvm_vblk_set_00_erase.cmd
   :language: bash

.. literalinclude:: nvm_vblk_set_00_erase.out
   :language: bash

Write
~~~~~

.. literalinclude:: nvm_vblk_set_01_write.cmd
   :language: bash

.. literalinclude:: nvm_vblk_set_01_write.out
   :language: bash

Read
~~~~

.. literalinclude:: nvm_vblk_set_02_read.cmd
   :language: bash

.. literalinclude:: nvm_vblk_set_02_read.out
   :language: bash

.. _sec-cli-vblk-line:

Block Line
----------

Construct a virtual block using block 10 in the ranges channel[0,0] and
LUN[0,3]. That is, Block ten in the first four LUNs of channel 0.

Erase
~~~~~

.. literalinclude:: nvm_vblk_line_00_erase.cmd
   :language: bash

.. literalinclude:: nvm_vblk_line_00_erase.out
   :language: bash

Write
~~~~~

.. literalinclude:: nvm_vblk_line_01_write.cmd
   :language: bash

.. literalinclude:: nvm_vblk_line_01_write.out
   :language: bash

Read
~~~~

.. literalinclude:: nvm_vblk_line_02_read.cmd
   :language: bash

.. literalinclude:: nvm_vblk_line_02_read.out
   :language: bash
