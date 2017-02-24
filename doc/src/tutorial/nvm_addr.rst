Physical Addressing
===================

Most of the library takes one or more physical addresses as parameter.

Generic format
--------------

The physical addresses are represented in generic format by the data-structure
``struct nvm_addr``. One can construct an address by specifying the relative
location within the device geometry down to the granularity of a sector.

Construct an address for sector 3 within page 11 in block 200 on plane 0 of LUN
1 in channel 4:

.. literalinclude:: nvm_addr_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_addr_00.out
   :language: bash

The above hexidecimal-value can be given to any CLI command taking an ``addr``
as parameter.

.. NOTE:: Addresses are zero-indexed, so channel 4 is the fifth channel

.. NOTE:: C API address construction is done by assigning the members of ``struct nvm_addr``

Device format
-------------

As the output from the device information shows, then there is a notion of a
device format. The library user need not be concerned with the device format as
the translation to device format is handled by the library for every part of
the interface with the exception of the low-level command-interface
``nvm_cmd``.

However, if one needs an address on device format for ``nvm_cmd`` or another
tool such as ``nvme-cli``, then the generic-format can be converted to
device format using:

.. literalinclude:: nvm_addr_01.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_addr_01.out
   :language: bash

.. NOTE:: C API address format conversion is done using ``nvm_addr_gen2dev``

Address scope
-------------

An address specifies the relative location of all parts of the geometry,
channel, lun, plane, block, page and sector. However, not all parts of the
library uses all location information. Most common uses are:

LUN address
  Specify channel and LUN within the channel

Block address
  Specify channel, LUN within the channel, plane within the LUN, and block
  within the plane

Sector address
  Specify all relative locations of the geometry

