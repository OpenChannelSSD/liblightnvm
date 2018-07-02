Obtaining device information
============================

To begin with, knowing the physical geometry of a device is essential for
working with physical addressing. Device information is obtained by invoking:

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

