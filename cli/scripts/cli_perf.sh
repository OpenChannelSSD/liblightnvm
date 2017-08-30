#!/usr/bin/env bash

function usage {
	echo "Usage: $0 DEV_PATH BLK"
	echo "Example: $0 /dev/nvme0n1 42"
	exit
}

DEV_PATH=$1
BLK=$2

if [ -z "$DEV_PATH" ] || [ -z "$BLK" ]; then
	usage
fi

NLUNS=8
NCHANNELS=16

CMDS="line_erase line_write line_read"

for CH_END in `seq 0 $(( $NCHANNELS - 1 ))`;
do
	CH_BGN=0
	LUN_BGN=0
	LUN_END=$(( $NLUNS - 1 ))

	CH_COUNT=$(( 1 + $CH_END - $CH_BGN ))

	echo ""
	echo "## erase/write/read on $DEV_PATH using $CH_COUNT channel(s)"

	for CMD in $CMDS
	do
		CMD_STR="nvm_vblk $CMD $DEV_PATH $CH_BGN $CH_END $LUN_BGN $LUN_END $BLK"
		#echo "# $CMD_STR"
		eval $CMD_STR | grep "nvm_vblk_"
		ERR=$?
		if [ $ERR -ne 0 ]; then
			echo "vblk $CMD failed, try another device and/or block"
			exit $ERR
		fi
	done
done
