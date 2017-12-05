#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import pprint
import glob
import json
import os
import re

def _execute(cmd):
    """Execute the given command and return stdout, stderr, and returncode"""

    process = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
    out, err = process.communicate()

    return out, err, process.returncode

def _update_file(fpath, content):

    with open(fpath, 'w+') as output:
        output.write(content)

def produce_usage_output(args):
    """Do the actual work"""

    # Construct usage output
    for usage_path in glob.glob(os.sep.join([args.path, "nvm_*_usage.out"])):
        cmd_fn = os.path.basename(usage_path).replace("_usage.out","")

        out, err, _ = _execute(cmd_fn)

        if "(%s_*)" % cmd_fn not in out:
            print("FAILED: executing(%s) => skipping update" % cmd_fn)
            continue

        _update_file(usage_path, out+err)

def produce_cmd_output(args):
    """Do the actual work"""

    # Construct command output

    for cmd_path in sorted(glob.glob(os.sep.join([args.path, "*.cmd"]))):
        output = []

        if args.spec and args.spec == "s20" and "bbt" in cmd_path:
            continue

        # Grab commands
        cmds = [line.strip() for line in open(cmd_path).readlines()]

        # Merge those line-continuations
        cmds = "\n".join(cmds).replace("\\\n", "").splitlines()

        for cmd in cmds:
            print(cmd_path, cmd)

            out, err, code = _execute(cmd)

            if code:
                print("FAILED: CMD(%s) CHECK THE OUTPUT" % cmd)

            output.append(out+err)

        _update_file(cmd_path.replace(".cmd", ".out"), "\n".join(output))

def expand_path(path):
    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))

if __name__ == "__main__":
    PRSR = argparse.ArgumentParser(
        description='Fills in usage and command results'
    )
    PRSR.add_argument(
        "path",
        type=str,
        help="Path to DIR containing *_usage and *.cmd files for CLI"
    )

    PRSR.add_argument(
        "--spec",
        type=str,
        default=None,
        help="Spec version"
    )

    ARGS = PRSR.parse_args()
    ARGS.path = expand_path(ARGS.path)

    try:
        produce_usage_output(ARGS)
        produce_cmd_output(ARGS)
    except OSError as EXC:
        print("FAILED: err(%s)" % EXC)
