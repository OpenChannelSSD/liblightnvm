.. _sec-cli-nvm_addr:

nvm_addr
========

.. literalinclude:: nvm_addr_usage.out
   :language: bash

.. tip:: See section :ref:`sec-cli-env` for a full list of environment
  variables modifying command behavior

Address Formats
---------------

The command-line parameter ``addr`` is a textual hexidecimal representation of
a physical address in generic format.

Generic Format
~~~~~~~~~~~~~~

The relative location in device geometry can be used to construct physical
address in generic format, e.g. channel(2), LUN(1), plane(0), blk(10),
page(200), sector(2):

.. literalinclude:: nvm_addr_from_geo.cmd
   :language: bash

.. literalinclude:: nvm_addr_from_geo.out
   :language: bash

Device Format
~~~~~~~~~~~~~

A physical address in device format can be construct from the physical address
in generic format:

.. literalinclude:: nvm_addr_gen2dev.cmd
   :language: bash

.. literalinclude:: nvm_addr_gen2dev.out
   :language: bash

Submitting IO
-------------

Four commands are provided for constructing read / write commands. The
write-command use a synthetic payload in the form of a repeating sequence of
chars A-Z.

write
  Perform a write of data using a synthetic payload at the given addresses

write_wm
  Perform a write of data and meta-data (out-of-bound area) using a synthetic
  payload at the given addresses

read
  Perform a read of data at the given addresses

read_wm
  Perform a read of data and meta-data (out-of-bound area) at the given
  addresses addresses

.. NOTE :: Commands by default expects plane-hint, however, it can be disabled

Perform a command without plane-hint by setting the environment var
``NVM_CLI_PMODE="0"``:

.. literalinclude:: nvm_addr_read_02.cmd
   :language: bash

.. literalinclude:: nvm_addr_read_02.out
   :language: bash

With plane-hint enabled perform a read command consisting of eight addresses:

.. literalinclude:: nvm_addr_read_00.cmd
   :language: bash

.. literalinclude:: nvm_addr_read_00.out
   :language: bash

Set the environment var ``NVM_CLI_BUF_PR`` to dump the read data to std:

.. literalinclude:: nvm_addr_read_01.cmd
   :language: bash

.. literalinclude:: nvm_addr_read_01.out
   :language: bash
   :lines: 1-15

.. code-block:: none

   ... output for bytes 128-32160 omitted for brevity ...

.. literalinclude:: nvm_addr_read_01.out
   :language: bash
   :lines: 1017-
