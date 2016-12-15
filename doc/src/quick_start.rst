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
  dev { path(/dev/nvme0n1), name(nvme0n1), fd(3) }
  dev-geo {
   nchannels(16), nluns(8), nplanes(2), nblocks(1048),
   npages(512), nsectors(4), sector_nbytes(4096),
   meta_nbytes(16),
   vpg_nbytes(32768b:32Kb),
   vblk_nbytes(16777216b:16Mb),
   total_nbytes(2250562863104b:2146304Mb)
  }
  dev-fmt {
   ch_ofz(0), ch_len(4),
   lun_ofz(7), lun_len(3),
   pl_ofz(6), pl_len(1),
   blk_ofz(19), blk_len(12),
   pg_ofz(10), pg_len(9),
   sec_ofz(4), sec_len(2)
  }

.. tip:: If the above does not suffice to get you started then have a look at the :ref:`sec-prereqs` for detailed information about what you need to setup an environment.

With the basics in place, explore the :ref:`sec-c-api`, the :ref:`sec-cli`, and
the :ref:`sec-python`. The section on :ref:`sec-background` provide a brief
glance of useful information on working with SSDs.

