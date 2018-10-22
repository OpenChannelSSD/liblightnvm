.. _sec-capi-nvm_ret:

nvm_ret - Return and Status Codes
=================================

The ``nvm_cmd`` functions signal errors using the standard ``errno``
thread-local variable and a return value of ``-1``. To gain more insights into
what went wrong in a command, use the optional nvm_ret parameter that all
``nvm_cmd`` functions take. If supplied, the ``nvm_ret`` will contain the NVMe
status code in the ``status`` member.

**Note** that while the :ref:`spdk <sec-backends-spdk>` backend has a
one-to-one correspondence between what is in the ``nvm_ret`` and what is
reported by the device, the :ref:`ioctl <sec-backends-ioctl>` backend will
transform certain errors from the kernel into their NVMe equivalents. This is
because certain ``ioctl`` calls may return ``-1`` and ``Invalid Argument``
without actually sending a command to the device. To align the backends with
each other, such an error wil be transformed into the NVMe equivalent if
possible. For example, ``Invalid Argument`` will be transformed into the NVMe
status code ``Invalid Field in Command`` and set that in the given ``nvm_ret``.

nvm_ret
-------

.. doxygenstruct:: nvm_ret
   :members:

nvm_ret_pr
----------

.. doxygenfunction:: nvm_ret_pr

