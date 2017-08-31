#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import sys
import re
from addrs import shell, gen_addrs

OPERATIONS=["set_erase", "set_write", "set_read"]

def perf(dev, nchannels, nluns, block):
	"""Run scalability experiment"""

	addrs = gen_addrs(dev, nchannels, nluns, block)

	for nluns in xrange(1, len(addrs) + 1):
		lun_addrs = addrs[0:nluns]

		result = {}

		for OPR in OPERATIONS:
			cmd = ["nvm_vblk", OPR, dev] + lun_addrs
			rc, out, err = shell(cmd)
			if rc:
				print("Failed executing: %s" % cmd)
				raise StopIteration

			res = re.search("mbsec: (\d+\.\d+)", out)
			if not res:
				print("Failed retrieving results")
				raise StopIteration

			result[OPR] = res.group(1)

		yield nluns, result

def main(args):
	"""Main entry point"""

	for nluns, result in perf(args.dev, args.nchannels, args.nluns, args.block):
		print(", ".join([str(nluns)] + [result[key] for key in OPERATIONS]))

	return 0

if __name__ == "__main__":

	PRSR = argparse.ArgumentParser(description='Generate some addresses')

	PRSR.add_argument('--dev', default="/dev/nvme0n1", help='Device to generate addresses')
	PRSR.add_argument('--nchannels', default=16, type=int, help='Number of channels to generate addrs. for')
	PRSR.add_argument('--nluns', default=8, type=int, help='Number of luns pr. channel to generate addrs. for')
	PRSR.add_argument('--block', default=0, type=int, help='The block to fix for the span of address')

	ARGS = PRSR.parse_args()
	
	sys.exit(main(ARGS))
