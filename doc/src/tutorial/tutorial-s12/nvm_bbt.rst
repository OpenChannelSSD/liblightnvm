Bad-Block-Table
===============

The state of the underlying media is exposed to the user through the
bad-block-interface (BBT). A bad-block table exists for each LUN, retrieving
the BBT for LUN 3 on channel 2 is done by executing:

.. literalinclude:: nvm_bbt_00_get.cmd
   :language: bash

The command outputs the entire table so we filter out the less interesting free
entries, thus yielding:

.. literalinclude:: nvm_bbt_00_get.out
   :language: bash

A given block can be in one of the following states: FREE, BAD, GROWN BAD,
DEVICE MARKED/RESERVED, and HOST MARKED/RESERVED. State can be changed by e.g.
marking block 42 as GROWN BAD:

.. literalinclude:: nvm_bbt_01_mark_g.cmd
   :language: bash

Yielding:

.. literalinclude:: nvm_bbt_01_mark_g.out
   :language: bash

.. NOTE :: SYSADMIN priviliges are required for modifying the BBT

Retrieving the BBT after the state change:

.. literalinclude:: nvm_bbt_02_get.cmd
   :language: bash

Yields:

.. literalinclude:: nvm_bbt_02_get.out
   :language: bash


.. NOTE :: C API for retrieving and modifying the BBT is done using
  ``nvm_bbt_get`` which return a ``const struct nvm_bbt*``. To modify it, make
  a copy with ``nvm_bbt_alloc_cp``, and persist it with ``nvm_bbt_set``.
