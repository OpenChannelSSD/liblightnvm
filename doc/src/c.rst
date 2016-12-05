.. _sec-c-api:

=======
 C API
=======

Bla bla bla asdfsd

.. toctree::

Geometry
========

NVM_GEO
-------

.. doxygenstruct:: NVM_GEO
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

NVM_DEV
-------

.. doxygentypedef:: NVM_DEV

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



Addressing
==========

NVM_ADDR
--------

.. doxygenstruct:: NVM_ADDR
   :members:

nvm_addr_erase
--------------

.. doxygenfunction:: nvm_addr_erase

nvm_addr_fmt_pr
---------------

.. doxygenfunction:: nvm_addr_fmt_pr

nvm_addr_mark
-------------

.. doxygenfunction:: nvm_addr_mark

nvm_addr_pr
-----------

.. doxygenfunction:: nvm_addr_pr

nvm_addr_read
-------------

.. doxygenfunction:: nvm_addr_read

nvm_addr_write
--------------

.. doxygenfunction:: nvm_addr_write



Virtual Block
=============

NVM_VBLK
--------

.. doxygentypedef:: NVM_VBLK

nvm_vblk_attr_addr
------------------

.. doxygenfunction:: nvm_vblk_attr_addr

nvm_vblk_new
------------

.. doxygenfunction:: nvm_vblk_new

nvm_vblk_new_on_dev
-------------------

.. doxygenfunction:: nvm_vblk_new_on_dev

nvm_vblk_free
-------------

.. doxygenfunction:: nvm_vblk_free

nvm_vblk_pr
-----------

.. doxygenfunction:: nvm_vblk_pr

nvm_vblk_get
------------

.. doxygenfunction:: nvm_vblk_get

nvm_vblk_gets
-------------

.. doxygenfunction:: nvm_vblk_gets

nvm_vblk_put
------------

.. doxygenfunction:: nvm_vblk_put

nvm_vblk_erase
--------------

.. doxygenfunction:: nvm_vblk_erase

nvm_vblk_pwrite
---------------

.. doxygenfunction:: nvm_vblk_pwrite

nvm_vblk_pread
--------------

.. doxygenfunction:: nvm_vblk_pread

nvm_vblk_read
-------------

.. doxygenfunction:: nvm_vblk_read

nvm_vblk_write
--------------

.. doxygenfunction:: nvm_vblk_write



Spanning Block
==============

NVM_SBLK
--------

.. doxygentypedef:: NVM_SBLK

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

nvm_sblk_erase
--------------

.. doxygenfunction:: nvm_sblk_erase

nvm_sblk_free
-------------

.. doxygenfunction:: nvm_sblk_free

nvm_sblk_new
------------

.. doxygenfunction:: nvm_sblk_new

nvm_sblk_pad
------------

.. doxygenfunction:: nvm_sblk_pad

nvm_sblk_pr
-----------

.. doxygenfunction:: nvm_sblk_pr

nvm_sblk_pread
--------------

.. doxygenfunction:: nvm_sblk_pread

nvm_sblk_pwrite
---------------

.. doxygenfunction:: nvm_sblk_pwrite

nvm_sblk_read
-------------

.. doxygenfunction:: nvm_sblk_read

nvm_sblk_write
--------------

.. doxygenfunction:: nvm_sblk_write


