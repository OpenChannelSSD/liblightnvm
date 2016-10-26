#!/usr/bin/env bash

BLK=$1
if [ -z "$BLK" ]; then
	echo "usage: $0 [SPANNED_BLK_IDX]"
	exit
fi

DRY=$2
if [ -z "$DRY" ]; then
	DRY="1"
fi

/usr/bin/time ./ewr_vpage.sh $BLK $DRY > ewr_vpage_${BLK}_01.log
/usr/bin/time ./ewr_vpage.sh $BLK $DRY > ewr_vpage_${BLK}_02.log

/usr/bin/time ./ewr_vpage_split.sh $BLK $DRY > ewr_vpage_split_${BLK}_01.log
/usr/bin/time ./ewr_vpage_split.sh $BLK $DRY > ewr_vpage_split_${BLK}_02.log

/usr/bin/time ./ewr_vblk.sh $BLK $DRY > ewr_vblk_${BLK}_01.log
/usr/bin/time ./ewr_vblk.sh $BLK $DRY  > ewr_vblk_${BLK}_02.log

/usr/bin/time ./ewr_vblk_split.sh $BLK $DRY > ewr_vblk_split_${BLK}_01.log
/usr/bin/time ./ewr_vblk_split.sh $BLK $DRY > ewr_vblk_split_${BLK}_02.log

