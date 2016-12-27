#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import pprint
import csv
import re

NCHANNELS=16
NLUNS=8

DEV="/dev/nvme0n1"
CHANNELS=range(0, NCHANNELS)
LUNS=range(0, NLUNS)
BLK=20
CLI=["span_erase", "span_write", "span_read"]
DRY=False

def main():

    results = []

    cfg = [(0, 0)] + [(ch, NLUNS-1) for ch in list(xrange(0, NCHANNELS))]

    for ch, lun in cfg:
        punits = (ch + 1) * (lun + 1)

        span = [str(x) for x in [0, ch, 0, lun, BLK]]

        for cli in CLI:
            op = cli.replace("span_", "")

            REGEX = "nvm_vblk_%s.*elapsed wall-clock: (\d+\.\d+)" % op

            cmd = ["nvm_vblk", cli, DEV] + span

            if DRY:
                print(cmd)
                continue

            process = Popen(cmd, stdout=PIPE, stderr=PIPE)
            out, err = process.communicate()

            matches = list(re.finditer(REGEX, out))
            if len(matches) != 1:
                print("SHOULD NOT HAPPEN")

            wc = float(matches[0].group(1))

            result = (op, punits + 1, "0-%d" % ch, "0-%d" % lun, wc)
            results.append(result)

            print("RAN: %s" % pprint.pformat(result))

    results = sorted(results)

    with open("/tmp/lun_scale.csv", "wb") as csv_fd:
        csv_writer = csv.writer(csv_fd)
        csv_writer.writerows(results)

if __name__ == "__main__":
    main()
