.. _sec-terminology:

Terminology
===========

Mostly, the terms used in liblightnvm are directly mapped to the OCSSD
specification(s). However, since the specifications have changed naming from
1.2 to 2.0, then a brief overview is provided here for reference as what a
liblightnvm term maps to in specifications.

+---------------+---------------+---------------------+
| liblightnvm   | OCSSD 1.2     | OCSSD 2.0           |
+===============+===============+=====================+
| PUGRP         | Channel       | Parallel Unit Group |
+---------------+---------------+---------------------+
| PUNIT         | LUN / die     | Parallel Unit       |
+---------------+---------------+---------------------+
| CHUNK         | Block         | Chunk               |
+---------------+---------------+---------------------+
| -             | Plane         | -                   |
+---------------+---------------+---------------------+
| -             | Page          | -                   |
+---------------+---------------+---------------------+
| SECTR         | Sector        | Logical Block       |
+---------------+---------------+---------------------+

Notice that from 1.2 to 2.0, the concept of BLOCK / PLANE / PAGE  is 'squashed'
into Chunk / Logical Block, however, liblightnvm refers to this as CHUNK/SECTR.
