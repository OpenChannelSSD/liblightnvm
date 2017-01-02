#!/usr/bin/env python
from __future__ import print_function
import argparse
import random
import lnvm

def main(args):

    dev = lnvm.Device(args.dev_path)
    if not dev.open():
        print("Failed opening dev_path(%s)" % args.dev_path)
        return

    geo = dev.geo()
    if not geo:
        print("Failed retrieving geometry")
        return

    blk = args.blk if args.blk else random.randint(10, geo.contents.nblocks)

    addrs = []

    for i in xrange(0, args.naddrs):
        addr = lnvm.Addr()

        addr.u.g.ch = i % geo.contents.nchannels
        addr.u.g.lun = (i / geo.contents.nchannels) % geo.contents.nluns
        addr.u.g.blk = blk

        ahex = "0x%016x" % addr.u.ppa
        addrs.append(ahex)

    dev.close()

    print(" ".join(addrs))

if __name__ == "__main__":
    PRSR  = argparse.ArgumentParser(description="Address construction")
    PRSR.add_argument(
        "dev_path",
        type=str,
        help="Path to Open-Channel SSD e.g. /dev/nvme0n1"
    )
    PRSR.add_argument(
        "naddrs",
        type=int,
        help="How many"
    )
    PRSR.add_argument(
        "--blk",
        type=int,
        help="Use a fixed blk instead of selecting on randomly"
    )

    ARGS = PRSR.parse_args()
    main(ARGS)
