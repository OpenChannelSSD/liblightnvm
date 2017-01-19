#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
from random import shuffle
import pprint

CMD = "nvm_vblk"
SUBS = ["line_erase", "line_write", "line_read"]

NCHANNELS = 16
NLUNS = 8
NBLOCKS = 1000

def main():

    lines = []
    for blk in xrange(0, NBLOCKS):
        lines.append((0, NCHANNELS-1, 0, NLUNS-1, blk))

    shuffle(lines)

    for line in lines:
        ch_bgn, ch_end, lun_bgn, lun_end, blk = line
        line_is_good = True

        print("Trying line(%s)" % pprint.pformat(line))

        for sub in SUBS:
            cmd = [
                CMD,
                sub,
                str(ch_bgn),
                str(ch_end),
                str(lun_bgn),
                str(lun_end),
                str(blk)
            ]
            process = Popen(cmd, stdout=PIPE, stderr=PIPE)
            out, err = process.communicate()
            if process.returncode:
                line_is_good = False
                break

        if line_is_good:
            print(line)
            break

if __name__ == "__main__":
    main()
