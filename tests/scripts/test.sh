#!/usr/bin/env bash

# Pick the environment
source env.sh

# Run the tests
sudo -E py.test nvm_test.py --junitxml /tmp/test_results.xml -r f --tb=line
