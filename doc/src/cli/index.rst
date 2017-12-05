.. _sec-cli:

========================
 Command-Line Interface
========================

The command-line interface (CLI) makes most of the :ref:`sec-c-api` available
from the command-line. The purpose of the CLI are manifold:

 * The CLI `source code <https://github.com/OpenChannelSSD/liblightnvm/tree/master/cli>`_ serve as example code of using the :ref:`sec-c-api`
 * The CLI itself provide tools for experimentation and debugging
 * A supplement to unit-testing the :ref:`sec-c-api`

Usage of the CLI is described by invoking the commands prefixed with ``nvm_``
without arguments and examples of their use are given in the following
sections.

Additionally, a set of :ref:`sec-cli-env` environment variables modify the
behavior of the CLI commands.

.. NOTE :: This CLI is not meant to be tool for conditioning or
  initialitization of Open-Channel SSDs, for such purposes see `lnvm-tools
  <https://github.com/OpenChannelSSD/lnvm-tools>`_ and `NVMe CLI
  <https://github.com/linux-nvme/nvme-cli>`_

.. toctree::
   :hidden:

   nvm_env
   nvm_dev
   nvm_addr
   nvm_cmd
   nvm_vblk
   nvm_bbt
