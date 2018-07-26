Device Media State
==================

Before submitting IO, then it is useful to obtain the state of the device
media, this can by done via the ``nvm_cmd rprt_all`` and ``nvm_cmd rprt_lun``.

.. literalinclude:: nvm_cmd_rprt_all.cmd
   :language: bash

Which will yield device information as shown below:

.. literalinclude:: nvm_cmd_rprt_all.out
   :language: bash
   :lines: 1-10

.. code-block:: none

   ... block address output omitted for brevity ...

.. literalinclude:: nvm_cmd_rprt_all.out
   :language: bash
   :lines: 47158-

The output is abbreviated as it contains states for all chunks.

.. NOTE:: The ``slba`` is on device-format use ``nvm_addr_dev2gen`` to convert
  it to general format.
