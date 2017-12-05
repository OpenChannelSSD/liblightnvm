.. _sec-quick-start:

=============
 Quick Start
=============

Assuming that you have met the :ref:`sec-prereqs`, and running Ubuntu Linux
(16.04/xenial), then retrieve the source code, configure, build, and install
it:

.. code-block:: bash

  # Retrieve liblightnvm
  git clone https://github.com/OpenChannelSSD/liblightnvm.git
  cd liblightnvm

  # Default configuration, build and install
  make configure
  make
  sudo make install

The above will configure liblightnvm, with default options, and install it by
copying headers, libraries and binaries into system paths.

BUILD: Changing configuration options
=====================================

The configuration options are named ``option_{on,off}``, consult the Makefile
to see available options. Apply an option by typing it before the ``configure``
target. For example:

.. code-block:: bash

  # Change configuration, build and install
  make spdk_on debug_on deb_on configure
  make
  sudo make install

This will enable the SPDK backend, debugging, and build Debian packages. NOTE:
``spdk_on`` expects SPDK to be available in ``/opt/spdk``.

In case you enable build of Debian packages via ``deb_on``, then you can modify
the ``make install`` step to install/uninstall using the Debian package:

.. code-block:: bash

  # Change configuration, build and install via package
  make spdk_on debug_on deb_on configure
  make
  sudo make install-deb

  # Conveniently remove liblightnvm by uninstalling the Debian package
  sudo make uninstall-deb

With liblightnvm built and installed as you see fit, try the following examples
of the :ref:`sec-c-api` and :ref:`sec-cli`.

API: Hello Open-Channel SSD
===========================

This "hello-world" example prints out device information of an Open-Channel
SSD. Include the liblightnvm header in your C/C++ source:

.. literalinclude:: hello.c
   :language: c

Compile it
~~~~~~~~~~

.. literalinclude:: hello_00.cmd
   :language: bash

.. tip:: If you compiled liblightnvm with SPDK, then add SPDK to the command
  above as well.

Run it
~~~~~~

.. literalinclude:: hello_01.cmd
   :language: bash

.. literalinclude:: hello_01.out
   :language: bash

CLI: Hello Open-Channel SSD
===========================

Most of the C API is wrapped in a suite of command-line interface (CLI) tools.
The equivalent of the above example is readily available from the
:ref:`sec-cli-nvm_dev` command:

.. literalinclude:: quick_start_00.cmd
   :language: bash

Which should output information similar to:

.. literalinclude:: quick_start_00.out
   :language: bash

.. tip:: If the above does not suffice to get you started then have a look at the :ref:`sec-prereqs` for detailed information about what you need to setup an environment.

With the basics in place, have a look at the :ref:`sec-background` section on
working with NAND media, follow the :ref:`sec-tutorial` to see the background
information put into practice, and dive deeper into :ref:`sec-c-api` and
experiment with :ref:`sec-cli`.
