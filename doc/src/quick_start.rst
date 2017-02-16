.. _sec-quick-start:

=============
 Quick Start
=============

Assuming that you have already attached and installed your Open-Channel SSD and
running Ubuntu Linux (16.04) here is what you need to take liblightnvm for a
spin:

.. code-block:: bash

  echo "deb https://dl.bintray.com/openchannelssd/debs xenial main" | sudo tee -a /etc/apt/sources.list
  sudo apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 379CE192D401AB61
  sudo apt-get update
  sudo apt-get install liblightnvm

Hello NVM Device
================

Include the liblightnvm header in your C/C++ source:

.. literalinclude:: hw.c
   :language: c

Then compile it using your e.g. `gcc`:

.. code-block:: bash

  gcc hw.c -llightnvm -o hw

.. tip:: liblightnvm is also available as a **static** library. So you can add `/usr/lib/liblightnvm.a` to your compiler target instead of linking dynamically.

Now, go ahead and run it:

.. code-block:: bash

  chmod +x hw
  ./hw

Interact via CLI
================

Check that it works by querying your device for information:

.. code-block:: bash

  nvm_dev info /dev/nvme0n1

Which should output information similar to:

.. code-block:: bash

  ** Device information  -- nvm_dev_pr **
  dev {
   verid(0x02), beid(0x00),
   path(/dev/nvme2n1), name(nvme2n1), fd(3),
   ssw(12), pmode(2),
   erase_naddrs_max(64), read_naddrs_max(64), write_naddrs_max(64),
   meta_mode(0),
   bbts_cached(0)
  },
  dev-geo {
   nchannels(16), nluns(8), nplanes(4),
   nblocks(1065), npages(256), nsectors(4),
   page_nbytes(16384), sector_nbytes(4096), meta_nbytes(16),
   tbytes(2287070085120b:2181120Mb),
  }
  dev-ppaf {
    ch_off(26),  ch_len(04),
   lun_off(23), lun_len(03),
    pl_off(02),  pl_len(02),
   blk_off(12), blk_len(11),
    pg_off(04),  pg_len(08),
   sec_off(00), sec_len(02),
  }
  dev-ppaf_mask {
    ch(0000000000000000000000000000000000111100000000000000000000000000),
   lun(0000000000000000000000000000000000000011100000000000000000000000),
    pl(0000000000000000000000000000000000000000000000000000000000001100),
   blk(0000000000000000000000000000000000000000011111111111000000000000),
    pg(0000000000000000000000000000000000000000000000000000111111110000),
   sec(0000000000000000000000000000000000000000000000000000000000000011)
  }

.. tip:: If the above does not suffice to get you started then have a look at the :ref:`sec-prereqs` for detailed information about what you need to setup an environment.

With the basics in place, explore the :ref:`sec-c-api`, the :ref:`sec-cli`, and
the :ref:`sec-python`. The section on :ref:`sec-background` provide a brief
glance of useful information on working with SSDs.

