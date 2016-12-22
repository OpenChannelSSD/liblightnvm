.. _sec-c-api:

=======
 C API
=======

C API to interact with an Open-Channel SSDs from user space.

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

nvm_bbt_free
------------

.. doxygenfunction:: nvm_bbt_free

nvm_bbt_pr
----------

.. doxygenfunction:: nvm_bbt_pr

nvm_bbt_state_pr
----------------

.. doxygenfunction:: nvm_bbt_state_pr



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

nvm_addr_read
-------------

.. doxygenfunction:: nvm_addr_read

nvm_addr_write
--------------

.. doxygenfunction:: nvm_addr_write

nvm_addr_check
--------------

.. doxygenfunction:: nvm_addr_check

nvm_addr_gen2lba
----------------

.. doxygenfunction:: nvm_addr_gen2off

nvm_addr_lba2gen
----------------

.. doxygenfunction:: nvm_addr_off2gen

nvm_addr_pr
-----------

.. doxygenfunction:: nvm_addr_pr



Logical Addressing
==================

nvm_lba_pread
-------------

.. doxygenfunction:: nvm_lba_pread

nvm_lba_pwrite
--------------

.. doxygenfunction:: nvm_lba_pwrite



Virtual Block
=============

nvm_vblk
--------

.. doxygenstruct:: nvm_vblk
   :members:

nvm_vblk_erase
--------------

.. doxygenfunction:: nvm_vblk_erase

nvm_vblk_read
-------------

.. doxygenfunction:: nvm_vblk_read

nvm_vblk_write
--------------

.. doxygenfunction:: nvm_vblk_write

nvm_vblk_pad
------------

.. doxygenfunction:: nvm_vblk_pad

nvm_vblk_pread
--------------

.. doxygenfunction:: nvm_vblk_pread

nvm_vblk_pwrite
---------------

.. doxygenfunction:: nvm_vblk_pwrite

nvm_vblk_alloc
--------------

.. doxygenfunction:: nvm_vblk_alloc

nvm_vblk_alloc_span
-------------------

.. doxygenfunction:: nvm_vblk_alloc_span

nvm_vblk_free
-------------

.. doxygenfunction:: nvm_vblk_free

nvm_vblk_pr
-----------

.. doxygenfunction:: nvm_vblk_pr

nvm_vblk_attr_addr
------------------

.. doxygenfunction:: nvm_vblk_attr_addr

nvm_vblk_attr_bgn
-----------------

.. doxygenfunction:: nvm_vblk_attr_bgn

nvm_vblk_attr_end
-----------------

.. doxygenfunction:: nvm_vblk_attr_end

nvm_vblk_attr_geo
-----------------

.. doxygenfunction:: nvm_vblk_attr_geo

nvm_vblk_attr_pos_read
----------------------

.. doxygenfunction:: nvm_vblk_attr_pos_read

nvm_vblk_attr_pos_write
-----------------------

.. doxygenfunction:: nvm_vblk_attr_pos_write



