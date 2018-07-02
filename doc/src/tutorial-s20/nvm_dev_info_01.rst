Device Attributes
=================

To begin with, knowing the geometry and write-contraints of a device is
essential for working with hierarchical addressing.

Raw
---

Device information is obtained by invoking the ``nvm_cmd idfy`` in the raw form:

.. literalinclude:: nvm_cmd_idfy.cmd
   :language: bash

Which will yield device information as shown below:

.. literalinclude:: nvm_cmd_idfy.out
   :language: bash

Derived
-------

Additional information, included derived attributes is obtained by ``nvm_dev
info``:

.. literalinclude:: nvm_dev_info.cmd
   :language: bash

Which will yield device information as shown below:

.. literalinclude:: nvm_dev_info.out
   :language: bash

The parts involved from the C API are: ``nvm_dev_open`` to obtain a device
handle, ``nvm_dev_pr`` to produce the output above, and lastly
``nvm_dev_close`` to terminate the handle properly.

When using the C API, values and structures are retrieved using the attribute
getters ``nvm_dev_get_*`` e.g. use ``nvm_dev_get_geo`` to obtain the geometry
of a given device.
