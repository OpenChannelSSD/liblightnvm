#!/bin/bash

NVME_DEVICE="nvme0n1"

echo "Checks starting - " `date`

echo "Raw I/O API tests"
sudo ./raw_test $NVME_DEVICE

echo "Append API tests"
sudo ./append_test $NVME_DEVICE

echo "Checks completed - " `date`
