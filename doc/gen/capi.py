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

    titles = {
        "nvm_geo": "Geometry",
        "nvm_buf": "Buffer Allocation",
        "nvm_dev": "Device Management",
        "nvm_addr": "Addressing",
        "nvm_cmd": "Raw Commands",
        "nvm_async": "Async. Controls",
        "nvm_sgl": "Scather/Gather Lists",
        "nvm_vblk": "Virtual Block",
        "nvm_bp": "Boilerplate",
        "nvm_bbt": "Bad-Block-Table"
    }
    docs = {}

    lib = {}
    for line in out.split("\n"):
        parts = (" ".join(line.split())).split(" ")[:2]
        if len(parts) < 2:
            continue

        name, kind = parts
        ns = "_".join(name.split("_")[:2])

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

        title = "%s - %s" % (ns, titles[ns]) if ns in titles else ns

        rst = "\n".join([
            ".. _sec-capi-%s:" % ns, "",
            title,
            "=" * len(title),
            "", ""
        ])

        if "typedefs" in lib[ns]:
            for typedef in lib[ns]["typedefs"]:
                rst += "\n".join([
                    typedef,
                    "-" * len(typedef), "",
                    ".. doxygentypedef:: %s" % typedef,
                    "", ""
                ])

        # Normalize handling of struct and externvar, prefer struct when
        # available

        jazz = []
        for mangler in ["struct", "externvar"]:
            if mangler in lib[ns]:
                for struct in lib[ns][mangler]:
                    if struct in [item[0] for item in jazz]:
                        continue
                    jazz.append((struct, mangler))

        for struct, _ in jazz:
            rst += "\n".join([
                struct,
                "-" * len(struct), "",
                ".. doxygenstruct:: %s" % struct,
                "   :members:"
                "", "", ""
            ])

        if "enum" in lib[ns]:
            for enum in lib[ns]["enum"]:
                rst += "\n".join([
                    enum,
                    "-" * len(enum), "",
                    ".. doxygenenum:: %s" % enum,
                    "", ""
                ])

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
            _update_file(os.sep.join([ARGS.path, "%s.rst" % NS]), RST[NS])

        #if RST:
        #    with open(ARGS.rst, "w") as RST_FD:
        #        RST_FD.write(RST)
    except OSError as EXC:
        print("FAILED: generating RST err(%s)" % EXC)
