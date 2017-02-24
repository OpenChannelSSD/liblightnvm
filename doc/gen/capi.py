#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import pprint
import json
import os
import re

def _update_file(fpath, content):

    with open(fpath, 'w+') as output:
        output.write(content)

def gen_capi(args):
    """Generate C API RST as string"""

    if not args.header:
        return ""

    cmd = ["ctags", "-x", "--c-kinds=fpsgx", args.header]

    process = Popen(cmd, stdout=PIPE, stderr=PIPE)
    out, err = process.communicate()

    if process.returncode:
        return ""

    docs = {}

    lib = {}
    for line in out.split("\n"):
        parts = (" ".join(line.split())).split(" ")[:2]
        if len(parts) < 2:
            continue

        name, kind = parts
        ns = name.split("_")[1]

        if ns not in lib:
            lib[ns] = {}

        if kind not in lib[ns]:
            lib[ns][kind] = []

        lib[ns][kind].append(name)

    for ns in lib:

        if "prototype" in lib[ns]:
            ordering = [
                "bbt_get", "bbt_set", "bbt_mark", "bbt_flush",
                "addr_erase", "addr_read", "addr_write", "addr_check",
                "addr_.*2",
                "lba_p?read", "lba_p?write",
                "vblk_erase", "vblk_p?read", "vblk_p?write", "vblk_pad",
                "_alloc", "_fill", "_free", "_pr",
                "_get_", "_set_"
            ]

            ordered = []
            for order in ordering:
                for func in lib[ns]["prototype"]:
                    if re.search(order, func):
                        if func not in ordered:
                            ordered.append(func)

            lib[ns]["prototype"] = list(
                set(lib[ns]["prototype"]) -
                set(ordered)
            ) + ordered

        header = "nvm_%s" % ns

        rst = "\n".join([
                header,
                "=" * len(header),
                ""
        ])

        if "typedefs" in lib[ns]:
            for typedef in lib[ns]["typedefs"]:
                rst += "\n".join([
                    typedef,
                    "-" * len(typedef), "",
                    ".. doxygentypedef:: %s" % typedef,
                    "", ""
                ])

        for mangler in ["struct", "externvar"]:
            if mangler in lib[ns]:
                for struct in lib[ns][mangler]:
                    rst += "\n".join([
                        struct,
                        "-" * len(struct), "",
                        ".. doxygenstruct:: %s" % struct,
                        "   :members:",
                        "", ""
                    ])
        """
        if "enum" in lib[ns]:
            for enum in lib[ns]["enum"]:
                rst += "\n".join([
                    enum,
                    "-" * len(enum), "",
                    ".. doxygenenum:: %s" % enum,
                    "", ""
                ])
        """

        if "prototype" in lib[ns]:
            for func in lib[ns]["prototype"]:
                rst += "\n".join([
                    func,
                    "-" * len(func), "",
                    ".. doxygenfunction:: %s" % func,
                    "", ""
                ])

        docs[ns] = rst

    return docs

def expand_path(path):
    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))

if __name__ == "__main__":
    PRSR = argparse.ArgumentParser(description='Generate C API docs')

    PRSR.add_argument(
        "path",
        type=str,
        help="Path to DIR containing C API RST src (also output dir)"
    )

    PRSR.add_argument(
        "--header",
        type=str,
        required=True,
        help="Path to liblightnvm.h"
    )

    ARGS = PRSR.parse_args()
    if ARGS.path:
        ARGS.path = expand_path(ARGS.path)
    if ARGS.header:
        ARGS.header = expand_path(ARGS.header)

    try:
        RST = gen_capi(ARGS)

        for NS in RST:
            _update_file(os.sep.join([ARGS.path, "nvm_%s.rst" % NS]), RST[NS])

        #if RST:
        #    with open(ARGS.rst, "w") as RST_FD:
        #        RST_FD.write(RST)
    except OSError as EXC:
        print("FAILED: generating RST err(%s)" % EXC)
