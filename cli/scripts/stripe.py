#!/usr/bin/env python
from __future__ import print_function

WS_MIN=12
NADDRS_MAX=32
LGEO_NBYTES=4096

def stripe(count, offset):

    sectr_nbytes = LGEO_NBYTES

    # Do striping in terms of "work units" akak WUN
    nsectr = count / sectr_nbytes
    sectr_bgn = offset / sectr_nbytes
    sectr_end = sectr_bgn + (count / sectr_nbytes) - 1

    cmd_nsectr_max = (NADDRS_MAX / WS_MIN) * WS_MIN

    print("cmd_nsectr_max: %r" % cmd_nsectr_max)
    print("nbytes: %r, nsectr: %r" % (count, nsectr))
    print("sectr_bgn: %r, sectr_end: %r" % (sectr_bgn, sectr_end))

    if nsectr % WS_MIN:
        print("Unaligned workload")

    if sectr_bgn % WS_MIN:
        print("unaligned offset")

    nchunks = 3

    for sectr_ofz in xrange(sectr_bgn, sectr_end + 1, cmd_nsectr_max):
        cmd_nsectr = min([sectr_end - sectr_ofz + 1, cmd_nsectr_max])

        print("cmd_nsectr: %02r" % cmd_nsectr)

        for idx in xrange(0, cmd_nsectr):
            sectr = sectr_ofz + idx
            wunit = sectr / WS_MIN
            rnd = wunit / nchunks

            chunk = wunit % nchunks
            chunk_sectr = sectr % WS_MIN + rnd * WS_MIN

            print("sectr: %r, wunit: %r, rnd: %r, chunk_sectr: %r, chunk: %r" % (
                sectr, wunit, rnd, chunk_sectr, chunk
            ))

def main():

    stripe(LGEO_NBYTES * 96, LGEO_NBYTES * WS_MIN *42)
    #stripe(LGEO_NBYTES * 48, LGEO_NBYTES * 24)

if __name__ == "__main__":
    main()
