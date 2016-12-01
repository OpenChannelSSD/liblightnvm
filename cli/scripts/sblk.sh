#!/usr/bin/env bash

function usage {
	echo "usage: $0 DEV_PATH CH_BGN CH_END LUN_BGN LUN_END BLK DRY \"[cmd .. [cmd]]\""
	echo "example: $0 /dev/nvme0n1 0 1 0 3 10 0 \"erase write read\""
	exit
}

DEV_PATH=$1
CH_BEGIN=$2
CH_END=$3
LUN_BEGIN=$4
LUN_END=$5
BLK=$6
DRY=$7
CMDS=$8

if [ -z "$DEV_PATH" ] || [ -z "$CH_BEGIN" ] || [ -z "$CH_END" ] || \
   [ -z "$LUN_BEGIN" ] || [ -z "$LUN_END" ] || [ -z "$BLK" ] || [ -z "$DRY" ]; then
	usage
fi

if [ -z "$CMDS" ]; then
	CMDS="erase write read"
	echo "No commands given, using default($CMDS)"
fi

SZ=$((${#DEV_PATH} -2))
NVME_DEV=`echo "$DEV_PATH" | cut -c-$SZ`

echo "**"
echo "** $DEV_PATH -- $CMDS"
echo "**"

for CMD in $CMDS
do
	echo "*"
	echo "* $CMD 'spanned' block"
	echo "*"
	CMD_STR="/usr/bin/time nvm_sblk $CMD $DEV_PATH $CH_BEGIN $CH_END $LUN_BEGIN $LUN_END $BLK"
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

