.. _sec-c-api:

=======
 C API
=======

C API to interact with an Open-Channel SSDs from user space.

.. toctree::

Geometry
========

nvm_geo
-------

.. doxygenstruct:: nvm_geo
   :members:

nvm_geo_pr
----------

.. doxygenfunction:: nvm_geo_pr



Buffer Management
=================

nvm_buf_alloc
-------------

.. doxygenfunction:: nvm_buf_alloc

nvm_buf_fill
------------

.. doxygenfunction:: nvm_buf_fill

nvm_buf_pr
----------

.. doxygenfunction:: nvm_buf_pr



Device Management
=================

nvm_dev
-------

.. doxygenstruct:: nvm_dev
   :members:

nvm_dev_open
------------

.. doxygenfunction:: nvm_dev_open

nvm_dev_close
-------------

.. doxygenfunction:: nvm_dev_close

nvm_dev_attr_geo
----------------

.. doxygenfunction:: nvm_dev_attr_geo

nvm_dev_pr
----------

.. doxygenfunction:: nvm_dev_pr



Bad Block Management
====================

nvm_bbt
-------

.. doxygenstruct:: nvm_bbt
   :members:

nvm_bbt_get
-----------

.. doxygenfunction:: nvm_bbt_get

nvm_bbt_set
-----------

.. doxygenfunction:: nvm_bbt_set

nvm_bbt_mark
------------

.. doxygenfunction:: nvm_bbt_mark

nvm_bbt_pr
----------

.. doxygenfunction:: nvm_bbt_pr



Physical Addressing
===================

nvm_addr
--------

.. doxygenstruct:: nvm_addr
   :members:

nvm_ret
-------

.. doxygenstruct:: nvm_ret
   :members:

nvm_addr_erase
--------------

.. doxygenfunction:: nvm_addr_erase

nvm_addr_write
--------------

.. doxygenfunction:: nvm_addr_write

nvm_addr_read
-------------

.. doxygenfunction:: nvm_addr_read

nvm_addr_pr
-----------

.. doxygenfunction:: nvm_addr_pr

nvm_addr_fmt_pr
---------------

.. doxygenfunction:: nvm_addr_fmt_pr

nvm_addr_check
--------------

.. doxygenfunction:: nvm_addr_check

nvm_addr_gen2dev
----------------

.. doxygenfunction:: nvm_addr_gen2dev



Logical Addressing
==================

nvm_lba_pwrite
--------------

.. doxygenfunction:: nvm_lba_pwrite

nvm_lba_pread
-------------

.. doxygenfunction:: nvm_lba_pread

nvm_addr_gen2lba
----------------

.. doxygenfunction:: nvm_addr_gen2lba

nvm_addr_lba2gen
----------------

.. doxygenfunction:: nvm_addr_lba2gen



Virtual Block (was spanning block)
==================================

nvm_sblk
--------

.. doxygenstruct:: nvm_sblk
   :members:

nvm_sblk_erase
--------------

.. doxygenfunction:: nvm_sblk_erase

nvm_sblk_read
-------------

.. doxygenfunction:: nvm_sblk_read

nvm_sblk_write
--------------

.. doxygenfunction:: nvm_sblk_write

nvm_sblk_pad
------------

.. doxygenfunction:: nvm_sblk_pad

nvm_sblk_pread
--------------

.. doxygenfunction:: nvm_sblk_pread

nvm_sblk_pwrite
---------------

.. doxygenfunction:: nvm_sblk_pwrite

nvm_sblk_alloc
--------------

.. doxygenfunction:: nvm_sblk_alloc

nvm_sblk_alloc_span
-------------------

.. doxygenfunction:: nvm_sblk_alloc_span

nvm_sblk_free
-------------

.. doxygenfunction:: nvm_sblk_free

nvm_sblk_pr
-----------

.. doxygenfunction:: nvm_sblk_pr

nvm_sblk_attr_end
-----------------

.. doxygenfunction:: nvm_sblk_attr_end

nvm_sblk_attr_geo
-----------------

.. doxygenfunction:: nvm_sblk_attr_geo

nvm_sblk_attr_pos_read
----------------------

.. doxygenfunction:: nvm_sblk_attr_pos_read

nvm_sblk_attr_pos_write
-----------------------

.. doxygenfunction:: nvm_sblk_attr_pos_write



Deprecated block interface
==========================

nvm_dblk
--------

.. doxygenstruct:: nvm_dblk
   :members:

nvm_dblk_erase
--------------

.. doxygenfunction:: nvm_dblk_erase

nvm_dblk_write
--------------

.. doxygenfunction:: nvm_dblk_write

nvm_dblk_read
-------------

.. doxygenfunction:: nvm_dblk_read

nvm_dblk_pwrite
---------------

.. doxygenfunction:: nvm_dblk_pwrite

nvm_dblk_pread
--------------

.. doxygenfunction:: nvm_dblk_pread

nvm_dblk_attr_addr
------------------

.. doxygenfunction:: nvm_dblk_attr_addr

nvm_dblk_pr
-----------

.. doxygenfunction:: nvm_dblk_pr

nvm_dblk_alloc
--------------

.. doxygenfunction:: nvm_dblk_alloc

nvm_dblk_alloc_span
-------------------

.. doxygenfunction:: nvm_dblk_alloc_span

nvm_dblk_free
-------------

.. doxygenfunction:: nvm_dblk_free


