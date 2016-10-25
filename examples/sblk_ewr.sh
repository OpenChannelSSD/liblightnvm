#!/usr/bin/env bash

BLK=$1
if [ -z "$BLK" ]; then
	echo "usage: ./sbl_ewr.sh [SPANNED_BLK_IDX]"
	exit
fi

NVME_DEV=nvme0
LNVM_DEV=nvme0n1
NCHANNELS=`cat /sys/class/nvme/$NVME_DEV/$LNVM_DEV/lightnvm/num_channels`
CH_BEGIN=0
CH_END=$(($NCHANNELS-1))
NLUNS=`cat /sys/class/nvme/$NVME_DEV/$LNVM_DEV/lightnvm/num_luns`
LUN_BEGIN=0
LUN_END=$(($NLUNS-1))
DRY=0

echo "** $LNVM_DEV with nchannels($NCHANNELS) and nluns($NLUNS)"

echo "** Erasing 'spanned' block"
for CH in $(seq $CH_BEGIN $CH_END); do
	for LUN in $(seq $LUN_BEGIN $LUN_END); do
		echo "*** ch($CH), lun($LUN), blk($BLK)"
		if [ $DRY -ne "1" ]; then
			nvm_vblk erase $LNVM_DEV $CH $LUN $BLK
		fi
	done
done

echo "** Writing spanned blk_idx($BLK) on $LNVM_DEV"
if [ $DRY -ne "1" ]; then
	nvm_sblk write $LNVM_DEV $BLK
fi

echo "** Reading spanned blk_idx($BLK) on $LNVM_DEV"
if [ $DRY -ne "1" ]; then
	nvm_sblk read $LNVM_DEV $BLK
fi
