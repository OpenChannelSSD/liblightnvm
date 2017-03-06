..
    Parts of the documentation is autogerated using doxygen, breathe, and
    custom Python scripts. See doc/gen/gen.py for the autogeneration logic,
    brief overview:
     * src/c.rst gen. by: doc/gen/gen.py, doc/gen/capi.json, doc/gen/capi.tmpl
     * src/cli.rst gen. by: doc/gen/gen.py, doc/gen/cli.json, doc/gen/cli.tmpl
    The rest of the documentation is written manually

==============================================
 User space I/O library for Open-Channel SSDs
==============================================

liblightnvm provides the means to interact with and obtain information about
Open-Channel SSDs from user space.

The core of the library provides an interface for performing vectorized I/O
using physical addressing. Command-construction is encapsulated, including
mapping a general physical addressing format to device specific. Thereby
allowing the user to focus on performing read/write on the vector space. The
interface is useful for experimenting with the vectorized I/O as provided by
Open-Channel SSDs. However, the library also support pealing of these
abstractions and let the user construct and submit "raw" commands.

For application integration, the "virtual block" interface provides a libc-like
``write``/``read``/``pread`` interface, encapsulating concerns of optimizing
command construction for utilization of parallel units on an Open-Channel SSD.
A virtual block can be created to span all parallel units or a subset thereof.

liblightnvm provides access to the user via a public C API, and a command-line
interface (CLI). The CLI is a thin wrapper around what the C API exposes and is
intended to provide easy access to what liblightnvm has to offer.

Here you can jump right into the :ref:`sec-quick-start` or take detailed look
at the :ref:`sec-prereqs` for using liblightnvm. :ref:`sec-background`
information is available, introducing common pitfalls when working with flash
memories.

With the basics in place you can explore the :ref:`sec-c-api` and the
:ref:`sec-cli`.

Contents:

.. toctree::
   :maxdepth: 2
   :includehidden:

   quick_start/index.rst
   prereqs/index.rst
   background/index.rst
   tutorial/index.rst
   capi/index.rst
   cli/index.rst
   refs/index.rst

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

