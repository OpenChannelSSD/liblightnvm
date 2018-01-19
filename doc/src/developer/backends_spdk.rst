SPDK
====

liblightnvm provides a kernel-bypassing backend implemented using `Intel SPDK
http://www.spdk.io/`_. On a recent Ubuntu LTS, run the following to install
SPDK from sources::

	# Make sure system is up to date
	sudo apt-get update && sudo apt-get upgrade && sudo apt-get dist-upgrade

	# Get SPDK
	sudo git clone https://github.com/spdk/spdk /opt/spdk
	sudo chown -R $USER:$USER /opt/spdk
	cd /opt/spdk
	git submodule update --init

	# Install dependencies
	sudo ./scripts/pkgdep.sh
	# This packages the script above misses
	sudo apt-get install uuid-dev
	./configure
	make -j $(nproc)

	# The final message at the end of the script indicates success or failure
	./unittest.sh

Output from the last command should at the end yield::

	=====================
	All unit tests passed
	=====================

Compiling Liblightnvm
---------------------

With SPDK in place, compile liblightnvm with::

	make spdk

Usage
-----

Unbinding devices
~~~~~~~~~~~~~~~~~

Run the following::

	sudo /opt/spdk/scripts/setup.sh

Device Identifiers
~~~~~~~~~~~~~~~~~~

Since devices are no longer available in `/dev`, then the PCI ids are used
using SPDK notation, such as `traddr:0000:01:00.0`, e.g. using the CLI::

  sudo nvm_dev info traddr:0000:01:00.0

And using the API it would be similar to::

  ...
  struct nvm_dev *dev = nvm_dev_open("traddr:0000:01:00.0");
  ...
