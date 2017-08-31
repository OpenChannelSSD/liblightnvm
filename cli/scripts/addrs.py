#!/usr/bin/env python
from __future__ import print_function
from subprocess import Popen, PIPE
import argparse
import sys
import re

def shell(cmd):
	"""Execute the given command"""

	cmd = [str(x) for x in cmd]

	process = Popen(cmd, stdout=PIPE, stderr=PIPE)

	out, err = process.communicate()

	return process.returncode, out, err

def gen_addrs(dev, nchannels, nluns, block):
	"""..."""
	
	tluns = nchannels * nluns

	addrs = []

	for idx in xrange(0, tluns):
		ch = idx % nchannels
		lun = (idx / nchannels) % nluns
		
		cmd = ["nvm_addr", "from_geo", dev, ch, lun, 0, block, 0, 0]

		rc, out, err = shell(cmd)
		if rc:
			return None

		res = re.search("ppa: (0x[a-zA-Z0-9]+)", out)
		if res:
			addrs.append(res.group(1))

	return addrs

def main(args):
	"""Main entry point"""

	addrs = gen_addrs(args.dev, args.nchannels, args.nluns, args.block)
	if addrs is None:
		return 1

	for addr in addrs:
		print(addr)

	return 0

if __name__ == "__main__":

	PRSR = argparse.ArgumentParser(description='Generate some addresses')

	PRSR.add_argument('--dev', default="/dev/nvme0n1", help='Device to generate addresses')
	PRSR.add_argument('--nchannels', default=16, type=int, help='Number of channels to generate addrs. for')
	PRSR.add_argument('--nluns', default=8, type=int, help='Number of luns pr. channel to generate addrs. for')
	PRSR.add_argument('--block', default=0, type=int, help='The block to fix for the span of address')

	ARGS = PRSR.parse_args()
	
	sys.exit(main(ARGS))
