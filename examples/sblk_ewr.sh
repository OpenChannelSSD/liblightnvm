#!/usr/bin/env bash

DEV=nvme0n1
BLK=$1
if [ -z "$BLK" ]; then
	echo "usage: ./erase.sh [SPANNED_BLK_IDX]"
	exit
fi

echo "** Erasing spanned blk_idx($BLK)"
for CH in {0..15}; do
	for LN in {0..7}; do
		nvm_vblk erase $DEV $CH $LN $BLK
	done
done

echo "** Writing spanned blk_idx($BLK)"
nvm_sblk write nvme0n1 $BLK

echo "** Reading spanned blk_idx($BLK)"
nvm_sblk read nvme0n1 $BLK
