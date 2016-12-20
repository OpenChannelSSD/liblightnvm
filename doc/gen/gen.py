#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import json
import os

def gen_cli(jsn, tmpl):
    """Generate CLI RST"""

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

def gen_capi(jsn, tmpl):
    """Generate C API RST as string"""

    for sect in jsn:

        rst = ""

        for typedef in sect["typedefs"]:
            rst += "\n".join([
                typedef,
                "-" * len(typedef), "",
                ".. doxygentypedef:: %s" % typedef,
                "", ""
            ])

        for struct in sect["structs"]:
            rst += "\n".join([
                struct,
                "-" * len(struct), "",
                ".. doxygenstruct:: %s" % struct,
                "   :members:",
                "", ""
            ])

        for enum in sect["enums"]:
            rst += "\n".join([
                enum,
                "-" * len(enum), "",
                ".. doxygenenum:: %s" % enum,
                "", ""
            ])

        for func in sect["functions"]:
            rst += "\n".join([
                func,
                "-" * len(func), "",
                ".. doxygenfunction:: %s" % func,
                "", ""
            ])

        tmpl = tmpl.replace("{{%s}}" % sect["name"].upper(), rst)

    return tmpl

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
        required=True,
        type=str,
        help="Path to input (e.g. doc/gen/api.json)"
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
    ARGS.json = os.path.abspath(ARGS.json)
    ARGS.tmpl = os.path.abspath(ARGS.tmpl)
    ARGS.rst = os.path.abspath(ARGS.rst)

    try:
        JSON = json.load(open(ARGS.json))
        TMPL = open(ARGS.tmpl).read()

        RST = CMDS[ARGS.topic](JSON, TMPL)

        with open(ARGS.rst, "w") as RST_FD:
            RST_FD.write(RST)
    except OSError as EXC:
        print("FAILED: generating RST(%s), err(%s)" % (ARGS.rst, EXC))
