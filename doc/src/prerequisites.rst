.. _sec-prereqs:

===============
 Prerequisites
===============

Here is what you need:

 * An Open-Channel SSD
 * Operating System Support
 * Management Tool

The following sections describe how to and options for meeting started
prerequisites.

An Open-Channel SDD
===================

Physical
--------

Aenean sit amet imperdiet lacus. Nulla arcu tortor, efficitur sit amet
pellentesque a, rhoncus id ante.


Virtual
-------

Virtual Open-Channel SSD devices are supported by the qemu-nvme
hosted on `GitHub <https://github.com/OpenChannelSSD/qemu-nvme>`_.

Install it by running:

.. code-block:: bash

  git clone https://github.com/OpenChannelSSD/qemu-nvme.git qemu-nvme
  cd qemu-nvme
  ./configure --target-list=x86_64-softmmu --enable-kvm --enable-linux-aio --enable-virtfs
  make
  sudo make install

Then use the following flags to setup a device:

.. code-block:: bash

  -device nvme,drive=$DRIVE_NAME,\
  serial=deadbeef,\
  namespaces=1,\
  mdts=10,\
  nlbaf=5,\
  lba_index=3\
  lbbtable=$DRIVE_BBT,\
  lmetadata=$DRIVE_METADATA,\
  lmetasize=$DRIVE_METASIZE

Operating System Support
========================

Linux you can use the kernel
sources available here:

 * Branch: liblnvm
 * Repos: git@github.com:OpenChannelSSD/linux.git

Management Tool
===============

nvme-cli

Aenean sit amet imperdiet lacus. Nulla arcu tortor, efficitur sit amet
pellentesque a, rhoncus id ante.


