.. _sec-backends-ioctl:

IOCTL
=====

The ``ioctl`` backend is a relatively simple backend that uses synchronous
ioctl calls to have the kernel perform NVMe commands.

The backend is also partially used by the :ref:`lbd <sec-backends-lbd>`
backend.


Note on Errors
--------------

Some commands may be issued through ``ioctl`` that are invalid. While most
commands will still be issued to the drive and the error relayed back to the
user, some violations may be picked up by the kernel cause the ``ioctl`` to
fail with ``-1`` and ``errno`` set to ``Invalid Argument``. In this case, the
backend will transform such errors into the NVMe equivalent. See the
documentation on :ref:`nvm_ret <sec-capi-nvm_ret>`.
