#!/usr/bin/env bash

NVM_DEV="traddr:0000:01:00.0"
#NVM_DEV="traddr:0000:02:00.0"
#NVM_DEV="/dev/nvme0n1"

select TASK in unbind build idf rprt bbt_get bbt_set blk01_read blk01_ewr blk02_ewr blk04_ewr blk08_ewr blk128_ewr;
do
	case $TASK in
	unbind)
		echo "## Unbinding NVMe devices"
		sudo /opt/spdk/scripts/setup.sh
		;;
	build)
		echo "## Building liblightnvm"
		make debug spdk
		#make spdk
		;;
	idf)
		echo "## Running nvm_dev info $NVM_DEV"
		sudo nvm_dev info $NVM_DEV
		;;
	rprt)
		echo "## Running nvm_cmd rprt $NVM_DEV"
		sudo nvm_cmd rprt $NVM_DEV
		;;
	bbt_get)
		echo "## Running nvm_bbt get $NVM_DEV"
		sudo valgrind nvm_bbt get $NVM_DEV 0 0
		;;
	bbt_set)
		echo "## Running nvm_bbt set $NVM_DEV"
		sudo nvm_bbt mark_b $NVM_DEV 0x0
		;;

	blk01_read)
		echo "## Running nvm_vblk read $NVM_DEV 0 0 0 0 0"
		sudo nvm_vblk line_read $NVM_DEV 0 0 0 0 0
		;;

	blk01_ewr)
		echo "## Running nvm_vblk erase $NVM_DEV 0 0 0 0 0"
		sudo nvm_vblk line_erase $NVM_DEV 0 0 0 0 0
		sudo nvm_vblk line_write $NVM_DEV 0 0 0 0 0
		sudo nvm_vblk line_read $NVM_DEV 0 0 0 0 0
		;;

	blk02_ewr)
		echo "## Running nvm_vblk erase $NVM_DEV 0 1 0 0 0"
		sudo nvm_vblk line_erase $NVM_DEV 0 1 0 0 0
		sudo nvm_vblk line_write $NVM_DEV 0 1 0 0 0
		sudo nvm_vblk line_read $NVM_DEV 0 1 0 0 0
		;;

	blk04_ewr)
		echo "## Running nvm_vblk erase $NVM_DEV 0 3 0 0 0"
		sudo nvm_vblk line_erase $NVM_DEV 0 3 0 0 0
		sudo nvm_vblk line_write $NVM_DEV 0 3 0 0 0
		sudo nvm_vblk line_read $NVM_DEV 0 3 0 0 0
		;;

	blk08_ewr)
		echo "## Running nvm_vblk erase $NVM_DEV 0 7 0 0 0"
		sudo nvm_vblk line_erase $NVM_DEV 0 7 0 0 0
		sudo nvm_vblk line_write $NVM_DEV 0 7 0 0 0
		sudo nvm_vblk line_read $NVM_DEV 0 7 0 0 0
		;;

	blk128_ewr)
		echo "## Running nvm_vblk erase $NVM_DEV 0 15 0 7 0"
		sudo nvm_vblk line_erase $NVM_DEV 0 15 0 7 0
		sudo nvm_vblk line_write $NVM_DEV 0 15 0 7 0
		sudo nvm_vblk line_read $NVM_DEV 0 15 0 7 0
		;;

	*)
		break
		;;
	esac
done

