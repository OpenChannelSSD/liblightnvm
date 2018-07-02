Hierarchical Addressing
=======================

Most of the library takes one or more hierarchical addresses as parameter.

Generic format
--------------

The hierarchical addresses are represented in generic format by the
data-structure ``struct nvm_addr``. One can construct an address by specifying
the relative location within the device geometry down to the granularity of a
sector.

Construct an address for sector 3 within chunk 142, within parallel unit 2,
within parallel group 4.

.. literalinclude:: nvm_addr_00.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_addr_00.out
   :language: bash

The above hexadecimal-value can be given to any CLI command taking an ``0xADDR``
as parameter.

.. NOTE:: Addresses are zero-indexed, so channel four is the fifth channel

.. NOTE:: C API address construction is done by assigning the members of ``struct nvm_addr``

Device format
-------------

As the output from the device information shows, then there is a notion of a
device format. The library user need not be concerned with the device format as
the translation to device format is handled by the library backend ``nvm_be``.

However, if one needs an address on device format for e.g. translating the
addresses listed by ``nvm_cmd_rprt`` or for another tool such as ``nvme-cli``,
then the generic-format can be converted to device format using:

.. literalinclude:: nvm_addr_01.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_addr_01.out
   :language: bash

Or from device format to generic format:

.. literalinclude:: nvm_addr_02.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_addr_02.out
   :language: bash

.. NOTE:: C API address format conversion is done using ``nvm_addr_gen2dev``
  and ``nvm_addr_dev2gen``.

Address scope
-------------

An address specifies the relative location of all parts of the geometry,
parallel unit group(PUGRP), parallel unit (PUNIT), chunk(CHUNK), and
sector(sectr).

PUNIT address
  Specify PUGRP and the PUNIT within the PUGRP

CHUNK address
  Specify PUGRP, PUNIT within the PUGRP, and CHUNK within the PUNIT

SECTR address
  Specify all relative locations of the geometry
