# liblightnvm smoketest

Install `py.test`:

    sudo apt-get install python-pytest

Set environment variables, e.g. for `NVM_BE_IOCTL`:

    export NVM_TEST_DEV_IDENT=/dev/nvme0n1
    export NVM_TEST_BE_ID=1

Set environment variables, e.g. for `NVM_BE_SPDK`:

    export NVM_TEST_DEV_IDENT=traddr:0000:01:00.0
    export NVM_TEST_BE_ID=4

Then run:

    py.test nvm_test.py --junitxml /tmp/test_results.xml -r f --tb=line

Or a subset, e.g.:

    py.test nvm_test.py -k "vblk" --junitxml /tmp/test_results.xml -r f --tb=line

You might need to prefix the commands with e.g. `sudo -E`.

    sudo -E py.test nvm_test.py \
        -k "erase" \
        --junitxml /tmp/test_results.xml \
        -r f \
        --tb=line

TODO: describe expected failures
