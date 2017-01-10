#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import pprint
import json
import os

def gen_cli(args):
    """Generate CLI RST"""

    if not args.json:
        return ""

    if not args.tmpl:
        return ""

    jsn = json.load(open(args.json))
    tmpl = open(args.tmpl).read()

    for sect in jsn:
        cmd = [sect["name"]]
        prc = Popen(cmd, stdout=PIPE, stderr=PIPE)
        usage, err = prc.communicate()

        usage = "\n" + usage
        usage = "\n  ".join(usage.split("\n"))
        rst = "\n".join([
            sect["name"],
            "=" * len(sect["name"]), "",
            ".. code-block:: bash",
            usage
        ])

        tmpl = tmpl.replace("{{%s}}" % sect["name"].upper(), rst)

    return tmpl

def gen_capi(args):
    """Generate C API RST as string"""

    if not args.header:
        return ""

    if not args.tmpl:
        return ""

    tmpl = open(args.tmpl).read()

    cmd = ["ctags", "-x", "--c-kinds=fpsgx", args.header]

    process = Popen(cmd, stdout=PIPE, stderr=PIPE)
    out, err = process.communicate()

    if process.returncode:
        return ""

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

        rst = ""

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

        tmpl = tmpl.replace("{{%s}}" % ns.upper(), rst)

    return tmpl

def expand_path(path):
    return os.path.abspath(os.path.expanduser(os.path.expandvars(path)))

if __name__ == "__main__":
    CMDS = {
        "capi": gen_capi,
        "cli": gen_cli
    }

    PRSR = argparse.ArgumentParser(description='Generate RST docs')
    PRSR.add_argument(
        "topic",
        choices=CMDS.keys(),
        type=str,
        help="Topic to generate RST for"
    )
    PRSR.add_argument(
        "--json", "-j",
        type=str,
        help="Path to input (e.g. doc/gen/cli.json)"
    )
    PRSR.add_argument(
        "--header",
        type=str,
        help="Path to liblightnvm.h"
    )
    PRSR.add_argument(
        "--tmpl", "-t",
        required=True,
        type=str,
        help="Path to template (e.g. doc/gen/api.tmpl)"
    )
    PRSR.add_argument(
        "--rst", "-r",
        required=True,
        type=str,
        help="Path to output (e.g. doc/src/c.rst)"
    )

    ARGS = PRSR.parse_args()
    if ARGS.json:
        ARGS.json = expand_path(ARGS.json)
    if ARGS.tmpl:
        ARGS.tmpl = expand_path(ARGS.tmpl)
    if ARGS.rst:
        ARGS.rst = expand_path(ARGS.rst)
    if ARGS.header:
        ARGS.header = expand_path(ARGS.header)

    try:
        RST = CMDS[ARGS.topic](ARGS)

        if RST:
            with open(ARGS.rst, "w") as RST_FD:
                RST_FD.write(RST)
    except OSError as EXC:
        print("FAILED: generating RST(%s), err(%s)" % (ARGS.rst, EXC))
