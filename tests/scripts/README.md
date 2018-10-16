# liblightnvm smoketest

Do this:

```bash
./test.sh
```

If this is littered with errors then look at the details below.

## Details

Install `py.test`:

```bash
sudo apt-get install python-pytest
```

Set environment variables, e.g. for `NVM_BE_IOCTL`:

```bash
export NVM_TEST_DEV_IDENT=/dev/nvme0n1
export NVM_TEST_BE_ID=1
```

Set environment variables, e.g. for `NVM_BE_SPDK`:

```bash
export NVM_TEST_DEV_IDENT=traddr:0000:01:00.0
export NVM_TEST_BE_ID=4
```

Or use the `env.sh` as:

```bash
source env.sh
```

Select the environment in the menu. This is just to make it easier to switch
between environments.

Then execute the test-runner:

```
py.test nvm_test.py --junitxml /tmp/test_results.xml -r f --tb=line
```

Or a subset of the tests, e.g. only `vblk` tests:

```
py.test nvm_test.py -k "vblk" --junitxml /tmp/test_results.xml -r f --tb=line
```

You might need to prefix the commands with `sudo -E` e.g.:

```bash
sudo -E [py.test nvm_test.py .... goes here]
```

TODO: describe expected failures
