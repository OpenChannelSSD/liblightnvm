.. _sec-cli-nvm_cmd:

nvm_cmd
=======

.. literalinclude:: nvm_cmd_usage.out
   :language: bash

.. tip:: See section :ref:`sec-cli-env` for a full list of environment
  variables modifying command behavior

Device Identification / Geometry
--------------------------------

For Open-Channel drives, both 1.2 and 2.0, then device identification and
geometry information can be quiried like so:

.. literalinclude:: nvm_cmd_idfy.cmd
   :language: bash

.. literalinclude:: nvm_cmd_idfy.out
   :language: bash

.. tip:: See section :ref:`sec-cli-nvm_dev` for additional device information.

Media State
-----------

For Open-Channel 1.2 drives then media state can be read and written via the
bad-block-table interface. See section :ref:`sec-cli-bbt` for an elaborate
description.

For Open-Channel 2.0 drives, the media state can only read, not written. This
is facilitated by the NVMe get-log-page, and in liblightnvm provided via the
following `nvm_cmd_rprt`.

To retrieve the entire state:

.. literalinclude:: nvm_cmd_rprt_all.cmd
   :language: bash

.. literalinclude:: nvm_cmd_rprt_all.out
   :language: bash

To retrieve the state for a single LUN / parallel unit:

.. literalinclude:: nvm_cmd_rprt_lun.cmd
   :language: bash

.. literalinclude:: nvm_cmd_rprt_lun.out
   :language: bash

Submitting IO
-------------

The ``nvm_cmd`` provides three IO sub-commands which constructs, submits and
waits for completion of a **single** NVMe command using the OCSSD erase(reset),
write(vector-write), and read(vector-read) command opcodes.

Perform a read command:

.. literalinclude:: nvm_cmd_read_00.cmd
   :language: bash

.. literalinclude:: nvm_cmd_read_00.out
   :language: bash

Use the ``-o FILE`` option, to dump the data read from device to file:

.. literalinclude:: nvm_cmd_read_01.cmd
   :language: bash

.. literalinclude:: nvm_cmd_read_01.out
   :language: bash

Which can then be inspected with for example ``hexdump``:

.. literalinclude:: nvm_cmd_read_03.cmd
   :language: bash

.. literalinclude:: nvm_cmd_read_03.out
   :language: bash
