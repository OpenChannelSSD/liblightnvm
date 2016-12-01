#!/usr/bin/env python
from __future__ import print_function
import json

def main():
    api = json.load(open('api.json'))
    tmpl = open('api.template').read()

    for sect in api:

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

        for func in sect["functions"]:
            rst += "\n".join([
                func,
                "-" * len(func), "",
                ".. doxygenfunction:: %s" % func,
                "", ""
            ])

        tmpl = tmpl.replace("{{%s}}" % sect["name"], rst)

    print(tmpl)

if __name__ == "__main__":
    main()
