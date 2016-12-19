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
   nvm_dev     info dev_path 
  

nvm_bbt
=======

.. code-block:: bash

  NVM bad-block-table (nvm_bbt_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_bbt      get dev_path ch lun
   nvm_bbt    set_f dev_path ch lun
   nvm_bbt    set_b dev_path ch lun
   nvm_bbt    set_g dev_path ch lun
   nvm_bbt   mark_f dev_path ppa [ppa...]
   nvm_bbt   mark_b dev_path ppa [ppa...]
   nvm_bbt   mark_g dev_path ppa [ppa...]
  

nvm_addr
========

.. code-block:: bash

  NVM address (nvm_addr_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_addr    erase dev_path ppa [ppa...]
   nvm_addr    write dev_path ppa [ppa...]
   nvm_addr     read dev_path ppa [ppa...]
   nvm_addr write_wm dev_path ppa [ppa...]
   nvm_addr  read_wm dev_path ppa [ppa...]
   nvm_addr  hex2gen dev_path ppa [ppa...]
   nvm_addr  gen2hex dev_path ch lun pl blk pg sec
   nvm_addr  hex2lba dev_path ppa [ppa...]
   nvm_addr  gen2lba dev_path ch lun pl blk pg sec
   nvm_addr  lba2gen dev_path lba [lba...]
  

nvm_lba
=======

.. code-block:: bash

  NVM logical-block-address (nvm_lba_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_lba   pwrite dev_path count offset
   nvm_lba    pread dev_path count offset
  

nvm_vblk
========

.. code-block:: bash

  NVM virtual block (nvm_vblk_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_vblk    erase dev_path ppa [ppa...]
   nvm_vblk    write dev_path ppa [ppa...]
   nvm_vblk     read dev_path ppa [ppa...]
   nvm_vblk   pwrite dev_path ppa [ppa...]
   nvm_vblk    pread dev_path ppa [ppa...]
  

nvm_sblk
========

.. code-block:: bash

  NVM spanning block (nvm_sblk_*) -- Ver { major(0), minor(0), patch(1) }
  
  Usage:
   nvm_sblk    erase dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_sblk    write dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_sblk      pad dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_sblk     read dev_path ch_bgn ch_end lun_bgn lun_end blk
   nvm_sblk chunked_w dev_path ch_bgn ch_end lun_bgn lun_end blk
  
