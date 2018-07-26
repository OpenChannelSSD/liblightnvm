.. _sec-backends:

==========
 Backends
==========

liblightnvm hides the implementation of operating system interaction from the
user. That is, the implementation of the ``nvm_cmd_*`` interface, which is
available in the public C API, is delegated at runtime to a backend
implementation.

+---------------+------------+
| Backend Name  | Identifier |
+===============+============+
| NVM_BE_IOCTL  | 0x1        |
+---------------+------------+
| NVM_BE_SYSFS  | 0x2        |
+---------------+------------+
| NVM_BE_LBA    | 0x4        |
+---------------+------------+
| NVM_BE_SPDK   | 0x8        |
+---------------+------------+

By default liblightnvm goes through the available backends in the order as
listed above and chooses to use the first backend capable of opening a device
without error.

The user can choose to use a specific backend by providing the backend
identifier. When using the CLI, this is done by setting the environment
variable NVM_CLI_BE_ID e.g.:

.. literalinclude:: nvm_be_cli.cmd
   :language: bash

.. literalinclude:: nvm_be_cli.out
   :language: bash

Or when using the ``C API``, by providing the backend identifier to function
`nvm_dev_openf`.

.. toctree::
   :hidden:

   nvm_be_ioctl
   nvm_be_lba
   nvm_be_sysfs
   nvm_be_spdk
