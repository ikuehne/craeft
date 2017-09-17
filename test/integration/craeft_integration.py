"""Script for running `craeftc` integration tests.

A craeftc integration test consists of three parts: a YAML configuration file, a
file containing Craeft code, a C harness, and a file containing expected output.
"""

import os
import subprocess
import tempfile
import traceback
import re

import yaml

DIR = os.path.dirname(__file__)
CRAEFT_PATH = os.path.join(DIR, '../../build/craeftc')
CC = "cc"

def temporary_filename():
    (obj, result) = tempfile.mkstemp()
    os.close(obj)
    return result

def try_rm(fname):
    try:
        os.remove(fname)
    except OSError:
        pass

def abs_of_conf_path(fname):
    return os.path.join(DIR, "tests", fname)

def assert_succeeded(args, msg):
    assert subprocess.call(args) == 0, msg

class IntegrationTest(object):

    def __init__(self, fname):
        """Parse the file named by `fname` into an IntegrationTest."""
        with open(fname, "r") as f:
            parsed = yaml.load(f)
        self.code = abs_of_conf_path(parsed["code"])
        self.harness = abs_of_conf_path(parsed["harness"])
        self.name = parsed["name"]

        with open(abs_of_conf_path(parsed["output"]), "r") as f:
            self.expected = bytes(f.read(), 'utf-8')

        self.code_obj = temporary_filename()
        self.harness_obj = temporary_filename()
        self.exc = temporary_filename()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        try_rm(self.code_obj)
        try_rm(self.harness_obj)
        try_rm(self.exc)

    def compile_craeft(self):
        assert_succeeded([CRAEFT_PATH, self.code, "--obj", self.code_obj],
                         "craeftc invocation failed")

    def compile_harness(self):
        assert_succeeded([CC, self.harness, "-c", "-o", self.harness_obj],
                         "compiler invocation failed")

    def link(self):
        assert_succeeded([CC, self.code_obj, self.harness_obj, "-o", self.exc],
                         "compiler linking invocation failed")

    def run_exc(self):
        child = subprocess.Popen([self.exc], stdout=subprocess.PIPE)
        assert child.wait() == 0, "executable failed"
        found = child.stdout.read()
        msg = "output incorrect: expected {}; found {}".format(self.expected,
                                                               found)
        assert found == self.expected, msg

    def run(self):
        self.compile_craeft()
        self.compile_harness()
        self.link()
        self.run_exc()

def main():
    contents = os.listdir(os.path.join(DIR, "tests"))
    confs = list(filter(re.compile(".*\.yaml$").match, contents))
    successes = 0
    print("Running tests...")
    for (i, conf) in enumerate(confs):
        fname = os.path.join(DIR, "tests", conf)
        with IntegrationTest(fname) as test:
            prefix = "test {}/{} ({}) ".format(i + 1, len(confs), test.name)
            try:
                test.run()
                successes += 1
                print(prefix + "succeeded.")
            except AssertionError as e:
                print(prefix + "failed. Stack trace:")
                traceback.print_exc()
    print("\nTests complete. {}/{} succeeded.".format(successes, len(confs)))

if __name__ == "__main__":
    main()
