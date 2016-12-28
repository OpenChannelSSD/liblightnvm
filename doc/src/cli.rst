.. _sec-cli:

========================
 Command-Line Interface
========================

The command-line interface (CLI) makes most of the :ref:`sec-c-api`
functionality available from the command-line. The purpose of the CLI are
manyfold:

 * The CLI source code serve as example code of using the :ref:`sec-c-api`
 * The CLI itself provide tools for experimentation and debugging
 * A supplement to unit-testing the :ref:`sec-c-api`

.. note:: This CLI is not meant to be tool for conditioning or initialitization of Open-Channel SSDs, for such purposes see `lvnm-tools <https://github.com/OpenChannelSSD/lnvm-tools>`_ and `NVMe CLI <https://github.com/linux-nvme/nvme-cli>`_

Usage of the CLI is described by invoking the commands without arguments, as
show below. And additionally by optionally defining the following environment
variables:

  * NVM_CLI_NOVERIFY -- When set, address verification is disabled
  * NVM_BUF_PR -- When set, read commands will dump the buffer to stdout
  * NVM_CLI_BUF_PR -- When set, read/write commands will dump buffer to stdout
  * NVM_CLI_META_PR -- When set, read/write commands will dump meta to stdout

.. toctree::

nvm_dev
=======

.. code-block:: bash

  NVM Device (nvm_dev_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_dev       info dev_path 
  

nvm_bbt
=======

.. code-block:: bash

  NVM bad-block-table (nvm_bbt_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_bbt        get dev_path ch lun
   nvm_bbt      set_f dev_path ch lun
   nvm_bbt      set_b dev_path ch lun
   nvm_bbt      set_g dev_path ch lun
   nvm_bbt      set_d dev_path ch lun
   nvm_bbt      set_h dev_path ch lun
   nvm_bbt     mark_f dev_path addr [addr...]
   nvm_bbt     mark_b dev_path addr [addr...]
   nvm_bbt     mark_g dev_path addr [addr...]
   nvm_bbt     mark_d dev_path addr [addr...]
   nvm_bbt     mark_h dev_path addr [addr...]
  

nvm_addr
========

.. code-block:: bash

  NVM address (nvm_addr_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_addr      erase dev_path addr [addr...]
   nvm_addr      write dev_path addr [addr...]
   nvm_addr       read dev_path addr [addr...]
   nvm_addr   write_wm dev_path addr [addr...]
   nvm_addr    read_wm dev_path addr [addr...]
   nvm_addr   from_hex dev_path addr [addr...]
   nvm_addr   from_geo dev_path ch lun pl blk pg sec
   nvm_addr    gen2dev dev_path addr [addr...]
   nvm_addr    gen2lba dev_path addr [addr...]
   nvm_addr    gen2off dev_path addr [addr...]
   nvm_addr    dev2gen dev_path num [num...]
   nvm_addr    lba2gen dev_path num [num...]
   nvm_addr    off2gen dev_path num [num...]
  

nvm_lba
=======

.. code-block:: bash

  NVM logical-block-address (nvm_lba_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_lba     pwrite dev_path count offset
   nvm_lba      pread dev_path count offset
  

nvm_vblk
========

.. code-block:: bash

  NVM Virtual Block (nvm_vblk_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_vblk      erase dev_path addr [addr...]
   nvm_vblk       read dev_path addr [addr...]
   nvm_vblk      write dev_path addr [addr...]
   nvm_vblk        pad dev_path addr [addr...]
   nvm_vblk  set_erase dev_path addr [addr...]
   nvm_vblk   set_read dev_path addr [addr...]
   nvm_vblk  set_write dev_path addr [addr...]
   nvm_vblk    set_pad dev_path addr [addr...]
   nvm_vblk line_erase dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_vblk  line_read dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_vblk line_write dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_vblk   line_pad dev_path ch_bgn ch_end lun_bgn lun_end blk
  
