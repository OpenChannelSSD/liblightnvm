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

.. literalinclude:: quick_start_00.cmd
   :language: bash

Which should output information similar to:

.. literalinclude:: quick_start_00.out
   :language: bash

.. tip:: If the above does not suffice to get you started then have a look at the :ref:`sec-prereqs` for detailed information about what you need to setup an environment.

With the basics in place, explore the :ref:`sec-c-api`, the :ref:`sec-cli`, and
the :ref:`sec-python`. The section on :ref:`sec-background` provide a brief
glance of useful information on working with SSDs.

