#!/bin/bash

NVME_DEVICE="nvme0n1"

echo "Checks starting - " `date`

sudo ./append_test $NVME_DEVICE

echo "Checks completed - " `date`
