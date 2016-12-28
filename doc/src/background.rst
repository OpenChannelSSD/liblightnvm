.. _sec-background:

============
 Background
============

A collection of notes which are nice to know when working with NAND flash
devices.

Device Architecture
===================

 * device geometry
 * parallel units
 * throughput and latency

Media Constraints
=================

Writes
------

Writes can be done at the granularity of a single physical page. In the example
geometry this means a minimum of four sector addresses must be constructed,
ordered from lowest to highest.

For a write to succeed then the page must reside in an erased block. This
constraint is often referred to as erase-before-write. Consequentially, one
cannot "update" a given flash page.

Reads
-----

Reads can be done at the granularity of a single sector. In the example
geometry this means a minimum of one sector address must be constructed.

For a read to success it must reside within a fully written block, sometimes
also referred to as a "closed block".

Alignment
---------

Media Access Mode
=================

Single vs multi-plane access mode (dual/quad)
