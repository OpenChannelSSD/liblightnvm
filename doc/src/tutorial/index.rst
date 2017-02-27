.. _sec-tutorial:

=======================
 Tutorial Introduction
=======================

This introduction will go through how to retrieve device information, construct
physical addresses, issue vectorized IO, retrieve and mange media state, and
introduce the virtual block abstraction.

It will be using the command-line interface (CLI), and provide notes for the
corresponding parts of the C API. It is assumed throughout that an Open-Channel
SSD available at the path ``/dev/nvme0n1`` and that liblightnvm is installed on
the system.

.. toctree::
   :hidden:

   nvm_dev.rst
   nvm_addr.rst
   nvm_addr_vio.rst
   nvm_vblk.rst
   nvm_bbt.rst
