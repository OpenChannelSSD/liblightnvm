#!/usr/bin/env python
"""
bintray descriptor file generator
"""
from __future__ import print_function
import argparse
import datetime
import json

def main(args):
    """Fill out descriptor with cli-arguments and dump it on stdout."""

    commit_short = args.commit[:8] if len(args.commit) > 8 else args.commit

    descr = {
        "package": {
            "name": "liblightnvm",
            "repo": args.repos,
            "subject": "openchannelssd",
            "desc": "liblightnvm - Userspace I/O library for LightNVM",
            "website_url": "openchannelssd.readthedocs.io",
            "issue_tracker_url":
                "https://github.com/OpenChannelSSD/liblightnvm/issues",
            "vcs_url": "https://github.com/OpenChannelSSD/liblightnvm.git",
            "github_use_tag_release_notes": False,
            "licenses": ["BSD 2-Clause"],
            "labels": ["nvm", "ssd", "flash"],
            "public_download_numbers": False,
            "public_stats": False,

        },
        "version": {
            "name": "%s-%s-%s" % (args.version, args.branch, commit_short),
            "desc": "Experimental/work-in-progress release",
            "released": args.rel_date,
            "vcs_tag": args.commit,
            "attributes": [
                {
                    "name": "branch",
                    "values": [args.branch],
                    "type": "string"
                },
            ],
            "gpgSign": False,
        },
        "publish": True
    }

    if args.repos == "binaries":
        descr["files"] = [
            {
                "includePattern": "./(liblightnvm)-(.*).tar.gz",
                "uploadPattern": "%s/$1-$2-%s-%s.tar.gz" % (
                    args.dist_code, args.branch, commit_short
                ),
                "matrixParams": {
                    "override": 1
                }
            }
        ]

    elif args.repos == "debs":
        descr["files"] = [
            {
                "includePattern": "./(liblightnvm)-(.*).deb",
                "uploadPattern": "%s/$1-$2-%s-%s.deb" % (
                    args.dist_code, args.branch, commit_short
                ),
                "matrixParams": {
                    "override": 1,
                    "deb_distribution": args.dist_code,
                    "deb_component": "main",
                    "deb_architecture": "amd64"
                }
            }
        ]
    else:
        raise ValueError("Invalid repos(%s)" % args.repos)

    print(json.dumps(descr, indent=2))

if __name__ == "__main__":
    PARSER = argparse.ArgumentParser(
        description="Produce a BinTray descriptor",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    PARSER.add_argument(
        "--repos",
        type=str,
        help="Name of repository, e.g. 'binaries' or 'liblightnvm'",
        choices=["binaries", "debs"],
        default="binaries"
    )
    PARSER.add_argument(
        "--dist_code",
        type=str,
        help="Distribution code e.g. xenial",
        required=True
    )
    PARSER.add_argument(
        "--version",
        type=str,
        help="Release version",
        default="1.0.0"
    )
    PARSER.add_argument(
        "--rel_date",
        type=str,
        help="Release date",
        default=datetime.datetime.now().strftime('%Y-%m-%d')
    )
    PARSER.add_argument(
        "--branch",
        type=str,
        help="Branch of git-repos",
        default="unk"
    )
    PARSER.add_argument(
        "--commit",
        type=str,
        help="Commit in git-repos",
        default="unk"
    )

    main(PARSER.parse_args())
