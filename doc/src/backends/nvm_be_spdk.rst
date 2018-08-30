SPDK
====

liblightnvm provides a kernel-bypassing backend implemented using `Intel SPDK
<http://www.spdk.io/>`_. On a recent Ubuntu LTS, run the following to install
SPDK from sources::

  # Make sure system is up to date
  sudo apt-get update && sudo apt-get upgrade && sudo apt-get dist-upgrade

  # Clone SPDK into /opt/spdk
  sudo git clone https://github.com/spdk/spdk /opt/spdk
  sudo chown -R $USER:$USER /opt/spdk

  # Go to the repository
  cd /opt/spdk
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

Output from the last `unittest.sh` command should yield::

  =====================
  All unit tests passed
  =====================

Compiling liblightnvm
---------------------

With `SPDK` in place, configure the liblightnvm build with::

  make spdk_on configure

Linking with `liblightnvm` and `SPDK`
-------------------------------------

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
    -luuid

The above compiles the example from the quick-start guide, note that the code
has a hardcoded device identifier, you must change this to match the `SPDK`
identifier.

Device Identifiers
~~~~~~~~~~~~~~~~~~

Since devices are no longer available in `/dev`, then the PCI ids are used
using SPDK notation, such as `traddr:0000:01:00.0`, e.g. using the CLI::

  sudo nvm_dev info traddr:0000:01:00.0

And using the API it would be similar to::

  ...
  struct nvm_dev *dev = nvm_dev_open("traddr:0000:01:00.0");
  ...

Unbinding devices
~~~~~~~~~~~~~~~~~

Run the following::

  sudo /opt/spdk/scripts/setup.sh

Re-binding devices
~~~~~~~~~~~~~~~~~~

Run the following::

  sudo /opt/spdk/scripts/setup.sh reset

