#!/usr/bin/env bash

DEFAULT_DEV_PATH=/dev/nvme0n1
DEFAULT_DEV_PCI=traddr:0000:01:00.0
DEFAULT_BE_ID=0

BE_NAMES=("ANY" "IOCTL" "LBD" "SPDK" "ZELDA" "Quit")

PS3='# Select OPT: '
select OPT in "${BE_NAMES[@]}"
do
  case $OPT in
    "ANY")
      echo "# You chose OPT: $OPT"
      NVM_TEST_DEV_IDENT=$DEFAULT_DEV_PATH
      NVM_TEST_BE_ID=$DEFAULT_BE_ID
      break
      ;;
    "IOCTL")
      echo "# You chose OPT: $OPT"
      NVM_TEST_DEV_IDENT=$DEFAULT_DEV_PATH
      NVM_TEST_BE_ID=1
      break
      ;;
    "LBD")
      echo "# You chose OPT: $OPT"
      NVM_TEST_DEV_IDENT=$DEFAULT_DEV_PATH
      NVM_TEST_BE_ID=2
      break
      ;;
    "SPDK")
      echo "# You chose OPT: $OPT"
      NVM_TEST_DEV_IDENT=$DEFAULT_DEV_PCI
      NVM_TEST_BE_ID=4
      echo "# Remember to run: 'sudo HUGEMEM=8192 /opt/spdk/scripts/setup.sh'"
      break
      ;;
    "ZELDA")
      echo "# You chose OPT: $OPT"
      NVM_TEST_DEV_IDENT=$DEFAULT_DEV_PCI
      NVM_TEST_BE_ID=8
      echo "# Remember to run: 'sudo HUGEMEM=8192 /opt/spdk/scripts/setup.sh'"
      break
      ;;
    "Quit")
      break
      ;;
    *) echo "# FAILED: Invalid OPT $REPLY";;
  esac
done

NVM_DEV_IDENT=$NVM_TEST_DEV_IDENT
NVM_CLI_BE_ID=$NVM_TEST_BE_ID

export NVM_TEST_DEV_IDENT
export NVM_TEST_BE_ID
export NVM_CLI_BE_ID
export NVM_DEV_IDENT

echo "# NVM_TEST_DEV_IDENT: '$NVM_TEST_DEV_IDENT', NVM_TEST_BE_ID: '$NVM_TEST_BE_ID'"
