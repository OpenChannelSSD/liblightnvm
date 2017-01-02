#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
from collections import deque
import argparse
import logging
import pprint
import csv
import re
import os
import lnvm

CLI = "nvm_vblk"

class Experiment(object):

    def __init__(self, args):

        self.subs = args.subs
        self.ops = [sub.replace("set_", "") for sub in self.subs]
        self.rotate = args.rotate
        self.naddrs_max = args.naddrs_max
        self.rpath = args.rpath
        self.dev_path = args.dev_path
        self.addrs = deque(args.addr)
        self.spans = {}
        self.trials = {op: [] for op in self.ops}

    def setup(self):
        """Setup spans and optionally rotates them to out wear"""

        for span in range(1, len(self.addrs)+1):
            span_addrs = list(self.addrs)[0:span]
            if self.rotate:
                self.addrs.rotate(-1)

            self.spans[span] = span_addrs

        logging.info("Got addrs(%s)", pprint.pformat(self.addrs))
        logging.info("Constructed span(%s)", pprint.pformat(self.spans))

        return True

    def run(self):
        """Run experiment"""

        env = os.environ.copy()

        env["NVM_CLI_ERASE_NADDRS_MAX"] = str(self.naddrs_max[0])
        env["NVM_CLI_READ_NADDRS_MAX"] = str(self.naddrs_max[1])
        env["NVM_CLI_WRITE_NADDRS_MAX"] = str(self.naddrs_max[2])

        logging.info("Running experiment")
        for span in self.spans:                         # Get addrs for the span

            addrs = " ".join(self.spans[span])

            for op, sub_cmd in zip(self.ops, self.subs):  # DO: erase/write/read
                REGEX = "nvm_vblk_%s, elapsed wall-clock: (\d+\.\d+)" % op

                cmd = " ".join([CLI, sub_cmd, self.dev_path, addrs])

                process = Popen(
                    cmd.split(" "),
                    stdout=PIPE,
                    stderr=PIPE,
                    env = env
                )
                out, err = process.communicate()

                if process.returncode:
                    logging.error(
                        "cmd(%s), returncode(%d), out(%s), err(%s)",
                        cmd,
                        process.returncode,
                        out,
                        err
                    )
                    return False

                matches = list(re.finditer(REGEX, out))
                if len(matches) != 1:
                    logging.error("Could not find elapsed time in output")
                    return False

                wall_clock = float(matches[0].group(1))
                res = (span, wall_clock)
                self.trials[op].append(res)

                logging.info("cmd(%s)", cmd)
                logging.info("res(%s)", res)

            self.to_file(self.rpath)

        logging.info("DONE")

        return True

    def to_file(self, path):
        """
        Write experiment and results to file

        @param path Absolute path to directory where results will be stored
        """

        for op in self.trials:
            rpath = os.sep.join([
                path, "%s-%02d_%02d_%02d.csv" % (
                    op,
                    self.naddrs_max[0],
                    self.naddrs_max[1],
                    self.naddrs_max[2]
                )
            ])
            with open(rpath, "wb") as csv_fd:
                csv_writer = csv.writer(csv_fd)
                csv_writer.writerows(self.trials[op])

def main(args):

    args.log = os.path.abspath(os.path.expanduser(os.path.expandvars(args.log)))
    args.rpath = os.path.abspath(os.path.expanduser(os.path.expandvars(args.rpath)))

    if not os.path.exists(os.path.dirname(args.log)):
        os.makedirs(os.path.dirname(args.log))

    if not os.path.exists(args.rpath):
        os.makedirs(args.rpath)

    logging.basicConfig(filename=args.log, level=logging.DEBUG)

    experiment = Experiment(args)
    experiment.setup()

    if not args.dry:
        if not experiment.run():
            print("Failure occured while running experiment")

if __name__ == "__main__":
    PRSR  = argparse.ArgumentParser(description="Run scalability experiment")
    PRSR.add_argument(
        "dev_path",
        type=str,
        help="Path to Open-Channel SSD e.g. /dev/nvme0n1"
    )
    PRSR.add_argument(
        "addr",
        type=str,
        nargs="+",
        help="List of addresses to use for scaling experiment"
    )
    PRSR.add_argument(
        "--subs",
        nargs="+",
        default=["set_erase", "set_write", "set_read"]
    )
    PRSR.add_argument(
        "--rotate",
        action="store_true",
        default=False,
        help="Rotate the list the list of address to even out wear"
    )
    PRSR.add_argument(
        "--naddrs_max",
        type=int,
        nargs=3,
        default=[64,64,64],
        help="Max number of addresses for ERASE READ WRITE"
    )
    PRSR.add_argument(
        "--dry",
        action="store_const",
        const=True,
        default=False
    )
    PRSR.add_argument(
        "--log",
        type=str,
        default="scale.log",
        help="Path to log-file"
    )
    PRSR.add_argument(
        "--rpath",
        type=str,
        default="results",
        help="Path to directory in which to store results"
    )

    ARGS = PRSR.parse_args()
    main(ARGS)
