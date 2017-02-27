.. _sec-prereqs-ocssd:

An Open-Channel SSD
===================

Physical
--------

A number of Open-Channel SSDs are available upon request. Public Open-Channel SSDs are

 * CNEX Labs LightNVM SDK
 * EMC Dragon Fire Board Open-Channel OX Controller
 * Radian Memory Systems RMS325

The CNEX Labs LightNVM SDK may be ordered for academical purposes. Please write to matias@cnexlabs.com with your research description and inquire for availability.

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
