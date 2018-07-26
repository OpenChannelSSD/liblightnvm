.. _sec-cli-bbt:

nvm_bbt
=======

.. literalinclude:: nvm_bbt_usage.out
   :language: none

.. tip:: See section :ref:`sec-cli-env` for a full list of environment
  variables modifying command behavior

.. SEEALSO:: See :ref:`sec-cli-nvm_addr` on how to construct the ``0xADDR`` parameter

Update block state
------------------

Set state of block 4 in LUN 2 in channel 1 as **grown bad**.

.. literalinclude:: nvm_bbt_00_mark_g.cmd
   :language: bash

.. literalinclude:: nvm_bbt_00_mark_g.out
   :language: bash

.. NOTE :: SYSADMIN priviliges are required to modify the BBT

Retrieve block state
--------------------

Retrieve the entire block state for all blocks in LUN 2 in Channel 1

.. literalinclude:: nvm_bbt_01_get.cmd
   :language: bash

.. literalinclude:: nvm_bbt_01_get.out
   :language: bash
   :lines: 1-15

.. code-block:: none

   ... output for entries 10 to 1013 omitted for brevity ...

.. literalinclude:: nvm_bbt_01_get.out
   :language: bash
   :lines: 1020-
