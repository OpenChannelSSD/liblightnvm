#!/usr/bin/env bash

function usage {
	echo "usage: $0 DEV_PATH CH LUN BLK DRY \"[cmd .. [cmd]]\""
	echo "example: $0 /dev/nvme0n1 0 0 10 0 \"erase write read\""
	exit
}

DEV_PATH=$1
CH=$2
LUN=$3
BLK=$4
DRY=$5
CMDS=$6

if [ -z "$DEV_PATH" ] || [ -z "$CH" ] || [ -z "$LUN" ] || \
   [ -z "$BLK" ] || [ -z "$DRY" ]; then
	usage
fi

if [ -z "$CMDS" ]; then
	CMDS="erase write read"
	echo "No commands given, using default($CMDS)"
fi

echo "**"
echo "** $DEV_PATH -- $CMDS"
echo "**"

for CMD in $CMDS
do
	echo "*"
	echo "* vblk: $CMD"
	echo "*"
	CMD_STR="/usr/bin/time nvm_deprecated_vblk $CMD $DEV_PATH $CH $LUN $BLK"
	echo "Invoking cmd($CMD_STR)"
	if [ $DRY -ne "1" ]; then
		eval $CMD_STR
		ERR=$?
		if [ $ERR -ne 0 ]; then
			echo "sblk operation error($ERR)"
			exit $ERR
		fi
	fi
done

