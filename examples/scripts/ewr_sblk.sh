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

echo "** E 'spanned' block"
if [ $DRY -ne "1" ]; then
	/usr/bin/time nvm_sblk erase $LNVM_DEV $CH_BEGIN $CH_END $LUN_BEGIN $LUN_END $BLK
	echo "nerr($?)"
fi

echo "** W 'spanned' blk($BLK) on $LNVM_DEV"
if [ $DRY -ne "1" ]; then
	/usr/bin/time nvm_sblk write $LNVM_DEV $CH_BEGIN $CH_END $LUN_BEGIN $LUN_END $BLK
	echo "nerr($?)"
fi

echo "** Reading spanned blk($BLK) on $LNVM_DEV"
if [ $DRY -ne "1" ]; then
	/usr/bin/time nvm_sblk read $LNVM_DEV $CH_BEGIN $CH_END $LUN_BEGIN $LUN_END $BLK
	echo "nerr($?)"
fi
