#!/usr/bin/env bash

BLK=$1
if [ -z "$BLK" ]; then
	echo "usage: ./sbl_ewr.sh [SPANNED_BLK_IDX]"
	exit
fi

NVME_DEV=nvme0
LNVM_DEV=nvme0n1
NCHANNELS=`cat /sys/class/nvme/$NVME_DEV/$LNVM_DEV/lightnvm/num_channels`
NLUNS=`cat /sys/class/nvme/$NVME_DEV/$LNVM_DEV/lightnvm/num_luns`

echo "** Erasing spanned blk_idx($BLK)"
for CH in {0..$(($NCHANNELS-1))}; do
	for LN in {0..$(($NLUNS-1))}; do
		nvm_vblk erase $LNVM_DEV $CH $LN $BLK
	done
done

echo "** Writing spanned blk_idx($BLK)"
nvm_sblk write nvme0n1 $BLK

echo "** Reading spanned blk_idx($BLK)"
nvm_sblk read nvme0n1 $BLK
