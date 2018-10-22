.. _sec-backends-lbd:

IOCTL - LBD
===========

The ``lbd`` backend issues commands to the device indirectly using the linux
block layer. Thus, regular ``read`` and ``write`` calls goes directly to the
NVMe block device (e.g. ``/dev/nvme0n1``) where the block layer transform the
calls into equivalent NVMe commands. Erases are implemented using the
``BLKDISCARD`` ``ioctl`` request.

Because the block layer by definition only supports scalar-based I/O, all other
commands (vector I/O, get and set features, chunk reporting) are redirected to
the :ref:`ioctl <sec-backends-ioctl>` backend.
