.. _sec-backends-spdk:

SPDK
====

**liblightnvm** provides a kernel-bypassing backend implemented using `Intel SPDK
<http://www.spdk.io/>`_.

Installing **SPDK**
-------------------

On a recent Ubuntu LTS, run the following to install **SPDK** from sources::

  # Make sure system is up to date
  sudo apt-get update && sudo apt-get upgrade && sudo apt-get dist-upgrade

  # Clone SPDK into /opt/spdk
  sudo git clone https://github.com/spdk/spdk /opt/spdk
  sudo chown -R $USER:$USER /opt/spdk

  # Go to the repository
  cd /opt/spdk

  # Checkout the v18.07 release
  git checkout v18.07
  git submodule update --init

  # Install dependencies
  sudo ./scripts/pkgdep.sh
  sudo apt-get install uuid-dev

  # Configure and build it
  ./configure
  make -j $(nproc)

  # Check that it works
  ./test/unit/unittest.sh

  # Install DPDK
  cd /opt/spdk/dpdk
  sudo make install

  # Install SPDK
  cd /opt/spdk
  sudo make install

Output from the last ``unittest.sh`` command should yield::

  =====================
  All unit tests passed
  =====================

Unbinding devices and setting up memory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By running the command below `8GB` of hugepages will be configured and the
device detached from the Kernel NVMe driver::

  sudo HUGEMEM=8192 /opt/spdk/scripts/setup.sh

This should output similar to::

  0000:01:00.0 (1d1d 2807): nvme -> vfio-pci

If anything else that the above is output from ``setup.sh``, for example::

  0000:01:00.0 (1d1d 2807): nvme -> uio_generic

Or::

  Current user memlock limit: 16 MB

  This is the maximum amount of memory you will be
  able to use with DPDK and VFIO if run as current user.
  To change this, please adjust limits.conf memlock limit for current user.

  ## WARNING: memlock limit is less than 64MB
  ## DPDK with VFIO may not be able to initialize if run as current user.

Then consult the notes on **Enabling ``VFIO`` without Limits**.

Re-binding devices
~~~~~~~~~~~~~~~~~~

Run the following::

  sudo /opt/spdk/scripts/setup.sh reset

Should output similar to::

  0000:01:00.0 (1d1d 2807): vfio-pci -> nvme

Device Identifiers
~~~~~~~~~~~~~~~~~~

Since devices are no longer available in ``/dev``, then the PCI ids are used
using **SPDK** notation, such as ``traddr:0000:01:00.0``, e.g. using the CLI::

  sudo nvm_dev info traddr:0000:01:00.0

And using the API it would be similar to::

  ...
  struct nvm_dev *dev = nvm_dev_open("traddr:0000:01:00.0");
  ...

Build **liblightnvm** with **SPDK** support
-------------------------------------------

With **SPDK** in place, configure the **liblightnvm** build with::

  make spdk_on configure build

Linking your source with **liblightnvm** and **SPDK**
-----------------------------------------------------

Invoke like so::

  gcc hello.c -o hello \
    -fopenmp \
    -llightnvm \
    -lspdk_nvme \
    -lspdk_util \
    -lspdk_log \
    -lspdk_env_dpdk \
    -lrte_bus_pci \
    -lrte_eal \
    -lrte_mempool \
    -lrte_mempool_ring \
    -lrte_pci \
    -lrte_ring \
    -lrt \
    -ldl \
    -lnuma \
    -luuid \
    -laio

The above compiles the example from the quick-start guide, note that the code
has a hardcoded device identifier, you must change this to match the **SPDK**
identifier.

Enabling ``VFIO`` without limits
--------------------------------

If ``nvme`` is rebound to ``uio_generic``, and not ``vfio``, then VT-d is
probably not supported or disabled. In either case try these two steps:

1) Verify that your CPU supports VT-d and that it is enabled in BIOS.

2) Enable your kernel by providing the kernel option `intel_iommu=on`.  If you
   have a non-Intel CPU then consult documentation on enabling VT-d / IOMMU for
   your CPU.

3) Increase limits, open ``/etc/security/limits.conf`` and add:

::

  *    soft memlock unlimited
  *    hard memlock unlimited
  root soft memlock unlimited
  root hard memlock unlimited

Once you have gone through these steps, then this command::

  dmesg | grep -e DMAR -e IOMMU

Should contain::

  [    0.000000] DMAR: IOMMU enabled

And this this command::

  find /sys/kernel/iommu_groups/ -type l

Should have output similar to::

  /sys/kernel/iommu_groups/7/devices/0000:00:1c.5
  /sys/kernel/iommu_groups/5/devices/0000:00:17.0
  /sys/kernel/iommu_groups/3/devices/0000:00:14.2
  /sys/kernel/iommu_groups/3/devices/0000:00:14.0
  /sys/kernel/iommu_groups/11/devices/0000:03:00.0
  /sys/kernel/iommu_groups/1/devices/0000:00:01.0
  /sys/kernel/iommu_groups/1/devices/0000:01:00.0
  /sys/kernel/iommu_groups/8/devices/0000:00:1d.0
  /sys/kernel/iommu_groups/6/devices/0000:00:1c.0
  /sys/kernel/iommu_groups/4/devices/0000:00:16.0
  /sys/kernel/iommu_groups/2/devices/0000:00:02.0
  /sys/kernel/iommu_groups/10/devices/0000:00:1f.6
  /sys/kernel/iommu_groups/0/devices/0000:00:00.0
  /sys/kernel/iommu_groups/9/devices/0000:00:1f.2
  /sys/kernel/iommu_groups/9/devices/0000:00:1f.0
  /sys/kernel/iommu_groups/9/devices/0000:00:1f.4

And **SPDK** setup::

  sudo HUGEMEM=8192 /opt/spdk/scripts/setup.sh

Should rebind the device to ``vfio-pci``, eg.::

  0000:01:00.0 (1d1d 2807): nvme -> vfio-pci

Inspecting and manually changing memory avaiable to `SPDK` aka `HUGEPAGES`
--------------------------------------------------------------------------

The `SPDK` setup script provides `HUGEMEM` and `NRHUGE` environment variables
to control the amount of memory available via `HUGEPAGES`. However, if you want
to manually change or just inspect the `HUGEPAGE` config the have a look below.

Inspect the system configuration by running::

  grep . /sys/devices/system/node/node0/hugepages/hugepages-2048kB/*

If you have not yet run the setup script, then it will most likely output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:0
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:0
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0

And after running the setup script it should output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:1024
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:1024
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0

This tells that `1024` hugepages, each of size `2048kB` are available, that is,
a total of two gigabytes can be used.

One way of increasing memory available to `SPDK` is by increasing the number of
`2048Kb` hugepages. E.g. increase from two to eight gigabytes by increasing
`nr_hugespages` to `4096`::

  echo "4096" > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages

After doing this, then inspecting the configuration should output::

  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/free_hugepages:4096
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages:4096
  /sys/devices/system/node/node0/hugepages/hugepages-2048kB/surplus_hugepages:0
