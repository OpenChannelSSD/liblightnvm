#!/usr/bin/env python
"""
 nvm_test - Turns nvm_test_* binaries `unittest` tests

 The manual invocation of the libcunit based tests are fine, however, by
 wrapping these in this runner-script allows for simpler reporting and
 instrumention when interfacing various CI environments

 Usage:

 # This will run nvm_test_* binaries, produce xml-output, basic console reporting
 py.test runner.py -k "foo"  --junitxml /tmp/test_results.xml -r f --tb=line

 # Run it with the framework build into Python (not useful for CI integration)
 ./runner.py

A bunch of other `unittest` compatible frameworks can be used, py.test, however,
seems to provide both decent xml and console output.

TODO: log output should be configurable/parameterizable

"""
from subprocess import Popen, STDOUT
import unittest
import glob
import os

BIN_PREFIX="nvm_test_"

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
        log = "/tmp/%s.log" % (bname)

        with open(log, "w+") as log_fd:

            cmd = [bname]
            process = Popen(cmd, stdout=log_fd, stderr=STDOUT)
            returncode = process.wait()

            assert process.returncode == 0, 'Test(%s/%s) exit(%d) consult(%s).' % (
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

def produce_classes(search_paths = None):
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
            lst[len(BIN_PREFIX):]
            for lst in os.listdir(path)
            if lst.startswith(BIN_PREFIX)
        ]
    tests = list(set(tests))

    for test in tests:
        cname = "Test%s" % "".join([t.capitalize() for t in test.split("_")])
        mname = "test_%s" % test

        cls = cls_factory(cname)
        setattr(cls, mname, cls.bin_test)
        classes[cname] = cls

    return classes

# Expose classes into scope for `unittest`, `py.test`, `nose`, `nose2`, or
# some other unittest framework to pick up the classes, instantiate
# them and run their test methods.
os.chdir(os.path.dirname(os.path.abspath(__file__)))

CLASSES = produce_classes() # Create test classes

for CNAME in CLASSES:           # Expose them in local scope
    CLS = CLASSES[CNAME]
    locals()[CNAME] = CLS
    del CLS                     # Avoid dual definition due to Python scope-leak

if __name__ == "__main__":      # When executed directly, use built-in runner
    unittest.main()
