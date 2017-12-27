#!/usr/bin/env python
"""
 nvm_test - Turns nvm_test_* binaries into `unittest.TestCase` classes.

 The manual invocation of the libcunit based tests are fine, however, by
 wrapping these in this runner-script allows for simpler reporting and
 instrumention when interfacing various CI environments

 Usage:

 # This will run nvm_test_vblk binaries, produce xml-output, basic console reporting
 py.test runner.py -k "vblk"  --junitxml /tmp/test_results.xml -r f --tb=line

 # Run it with the framework build into Python (not useful for CI integration)
 ./runner.py

    NVM_TEST_DEV
    NVM_TEST_LOG
    NVM_TEST_SEED
    NVM_TEST_BIN_PREFIX

TODO: log output should be configurable/parameterizable

"""
from subprocess import Popen, STDOUT
import unittest
import os

NVM_TEST_DEV = os.getenv("NVM_TEST_DEV", "/dev/nvme0n1")
NVM_TEST_LOG = os.getenv("NVM_TEST_LOG", os.sep + "tmp")
NVM_TEST_SEED = os.getenv("NVM_TEST_SEED", "0")
NVM_TEST_BIN_PREFIX = os.getenv("NVM_TEST_BIN_PREFIX", "nvm_test_")

# We inherit the large amount of public methods from unittest.TestCase,
# nothing we can do about that so we disable pylint checking.
# pylint: disable=r0904
class MetaTest(unittest.TestCase):
    """
    This is a meta-class used as the basis for dynamically
    creating unittest.TestCase classes based on .sh files.
    """

    def bin_test(self):
        """
        bin_test: executes libcunit binaries and pipes out+err to file
        """

        # Use naming conventions of Class and Method to obtain path
        _, cname, mname = self.id().split(".")
        if not cname.startswith("Test"):
            raise Exception("Invalid cname(%s)" % cname)
        if not mname.startswith("test_"):
            raise Exception("Invalid mname(%s)" % mname)

        bname = "nvm_%s" % mname

        # Run the actual test-script and pipe stdout+stderr to log
        log = os.sep.join([NVM_TEST_LOG, "%s.log" % (bname)])

        with open(log, "w+") as log_fd:

            cmd = [bname, NVM_TEST_DEV, NVM_TEST_SEED]
            process = Popen(cmd, stdout=log_fd, stderr=STDOUT)
            returncode = process.wait()

            assert process.returncode == 0, 'Test(%s/%s) exit(%d) log(%s).' % (
                cname, mname, returncode, log
            )

def cls_factory(cname):
    """
    Constructs a class named `cname` inheriting from `MetaTest`.
    """

    class Foo(MetaTest):
        """cls_factory product"""

        pass

    Foo.__name__ = cname

    return Foo

def produce_classes(search_paths=None):
    """
    Search all paths in $PATH for binaries prefixed with "nvm_test" and
    construct test classes for them.
    """

    if search_paths is None:
        search_paths = os.environ["PATH"].split(":")

    classes = {}

    tests = []
    for path in search_paths:
        if not os.path.exists(path):
            continue

        tests += [
            lst[len(NVM_TEST_BIN_PREFIX):]
            for lst in os.listdir(path)
            if lst.startswith(NVM_TEST_BIN_PREFIX)
        ]
    tests = list(set(tests))

    cname = "TestNvm"
    classes[cname] = cls_factory(cname)

    for test in tests:
        mname = "test_%s" % test

        setattr(classes[cname], mname, classes[cname].bin_test)

    return classes

# Expose classes into scope for `unittest`, `py.test`, `nose`, `nose2`, or
# some other unittest framework to pick up the classes, instantiate
# them and run their test methods.
os.chdir(os.path.dirname(os.path.abspath(__file__)))

CLASSES = produce_classes()     # Create test classes
for CNAME in CLASSES:           # Expose them in local scope
    CLS = CLASSES[CNAME]
    locals()[CNAME] = CLS
    del CLS                     # Avoid dual definition due to Python scope-leak

if __name__ == "__main__":      # When executed directly, use built-in runner
    unittest.main()
