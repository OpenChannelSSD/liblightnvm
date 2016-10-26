#!/usr/bin/env bash

BLK=$1
if [ -z "$BLK" ]; then
	echo "usage: $0 [BLK_IDX] [0|1]"
	exit
fi

DRY=$2
if [ -z "$DRY" ]; then
	DRY="1"
fi

NVME_DEV=nvme0
LNVM_DEV=nvme0n1
NCHANNELS=`cat /sys/class/nvme/$NVME_DEV/$LNVM_DEV/lightnvm/num_channels`
CH_BEGIN=0
CH_END=$(($NCHANNELS-1))
NLUNS=`cat /sys/class/nvme/$NVME_DEV/$LNVM_DEV/lightnvm/num_luns`
LUN_BEGIN=0
LUN_END=$(($NLUNS-1))

echo "** $LNVM_DEV with nchannels($NCHANNELS) and nluns($NLUNS)"

echo "** E 'spanned' block"
for CH in $(seq $CH_BEGIN $CH_END); do
	for LUN in $(seq $LUN_BEGIN $LUN_END); do
		echo "*** ch($CH), lun($LUN), blk($BLK)"
		if [ $DRY -ne "1" ]; then
			nvm_vblk erase $LNVM_DEV $CH $LUN $BLK
		fi
	done
done

echo "** W 'spanned' block"
for CH in $(seq $CH_BEGIN $CH_END); do
	for LUN in $(seq $LUN_BEGIN $LUN_END); do
		echo "*** ch($CH), lun($LUN), blk($BLK)"
		if [ $DRY -ne "1" ]; then
			nvm_vblk write $LNVM_DEV $CH $LUN $BLK
		fi
	done
done

echo "** R 'spanned' block"
for CH in $(seq $CH_BEGIN $CH_END); do
	for LUN in $(seq $LUN_BEGIN $LUN_END); do
		echo "*** ch($CH), lun($LUN), blk($BLK)"
		if [ $DRY -ne "1" ]; then
			nvm_vblk read $LNVM_DEV $CH $LUN $BLK
		fi
	done
done
