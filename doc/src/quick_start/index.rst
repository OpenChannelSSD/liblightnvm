.. _sec-quick-start:

=============
 Quick Start
=============

Assuming that you have met the :ref:`sec-prereqs`, and running Ubuntu Linux
(16.04), here is what you need to take liblightnvm for a spin:

.. code-block:: bash

  git clone https://github.com/OpenChannelSSD/liblightnvm.git
  cd liblightnvm
  make dev

With the library installed, try the following examples of the :ref:`sec-c-api`
and :ref:`sec-cli`.

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

.. tip:: liblightnvm is also available as a **static** library. So you can add
  `/usr/lib/liblightnvm.a` to your compiler target instead of linking
  dynamically.

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
