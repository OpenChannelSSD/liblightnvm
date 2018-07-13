.. _sec-cli-nvm_addr:

nvm_addr
========

.. literalinclude:: nvm_addr_usage.out
   :language: bash

.. tip:: See section :ref:`sec-cli-env` for a full list of environment
  variables modifying command behavior

Address Formats
---------------

The command-line parameter ``0xADDR`` is a textual hexadecimal representation
of a physical address in generic format.

Generic Format
~~~~~~~~~~~~~~

The relative location in device geometry can be used to construct physical
address in generic format, e.g. pugrp(2), punit(1), chunk(0), sectr(10):

.. literalinclude:: nvm_addr_s20_to_gen.cmd
   :language: bash

.. literalinclude:: nvm_addr_s20_to_gen.out
   :language: bash

Device Format
~~~~~~~~~~~~~

A physical address in device format can be construct from the physical address
in generic format:

.. literalinclude:: nvm_addr_gen2dev.cmd
   :language: bash

.. literalinclude:: nvm_addr_gen2dev.out
   :language: bash

Terse Format
~~~~~~~~~~~~

Prints the just the address:

.. literalinclude:: nvm_addr_tex01.cmd
   :language: bash

.. literalinclude:: nvm_addr_tex01.out
   :language: bash

.. literalinclude:: nvm_addr_tex02.cmd
   :language: bash

.. literalinclude:: nvm_addr_tex02.out
   :language: bash
