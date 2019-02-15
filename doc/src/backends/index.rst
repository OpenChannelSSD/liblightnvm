.. _sec-backends:

==========
 Backends
==========

liblightnvm hides the implementation of operating system interaction from the
user. That is, the implementation of the ``nvm_cmd_*`` interface, which is
available in the public C API, is delegated at runtime to a backend
implementation.

+------------------+------------+
| Backend Name     | Identifier |
+==================+============+
| ``NVM_BE_IOCTL`` | ``0x1``    |
+------------------+------------+
| ``NVM_BE_LBD``   | ``0x2``    |
+------------------+------------+
| ``NVM_BE_SPDK``  | ``0x4``    |
+------------------+------------+
| ``NVM_BE_PRXY``  | ``0x8``    |
+------------------+------------+

By default liblightnvm goes through the available backends in the order as
listed above and chooses to use the first backend capable of opening a device
without error.

The user can choose to use a specific backend by providing the backend
identifier via the API `nvm_dev_openf`. Or by setting environment variable
`NVM_BE` to the name of the backend. For example, when using the CLI:

.. literalinclude:: nvm_be_cli.cmd
   :language: bash

.. literalinclude:: nvm_be_cli.out
   :language: bash

Not all backends support all features.

+----------------------------+----------------------------------------------------+
|                            | Backends                                           |
+----------------------------+----------+-----------+-----------------------------+
| Feature                    | ``spdk`` | ``ioctl`` | ``lbd``                     |
+============================+==========+===========+=============================+
| Scalar I/O                 | **yes**  | **yes**   | **yes**                     |
+----------------------------+----------+-----------+-----------------------------+
| Scalar I/O *(w/ metadata)* | **yes**  | **yes**   | **no**                      |
+----------------------------+----------+-----------+-----------------------------+
| Vector I/O                 | **yes**  | **yes**   | **yes** (through ``ioctl``) |
+----------------------------+----------+-----------+-----------------------------+
| Vector I/O *(w/ metadata)* | **yes**  | **yes**   | **yes** (through ``ioctl``) |
+----------------------------+----------+-----------+-----------------------------+
| SGLs                       | **yes**  | **no**    | **no**                      |
+----------------------------+----------+-----------+-----------------------------+
| Async                      | **yes**  | **no**    | **partial** (only scalar)   |
+----------------------------+----------+-----------+-----------------------------+

.. toctree::
   :hidden:

   nvm_be_ioctl
   nvm_be_lbd
   nvm_be_spdk
   nvm_be_proxy
