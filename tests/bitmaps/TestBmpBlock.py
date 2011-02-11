#!/usr/bin/python -tt
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for bmpblk_utility.
"""

import os
import sys
import subprocess
import unittest

def runprog(*args):
  """Runs specified program and args, returns (exitcode, stdout, stderr)."""
  p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  out, err = p.communicate()
  return (p.returncode, out, err)


class TestBmpBlock(unittest.TestCase):

  def testNoArgs(self):
    """Running with no args should print usage and fail."""
    rc, out, err = runprog(prog)
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("missing BMPBLOCK name"))
    self.assertTrue(out.count("bmpblk_utility"))

  def testMissingBmp(self):
    """Missing a bmp specified in the yaml is an error."""
    rc, out, err = runprog(prog, '-c', 'case_nobmp.yaml', 'FOO')
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("No such file or directory"))

  def testInvalidBmp(self):
    """A .bmp file that isn't really a BMP should fail."""
    rc, out, err = runprog(prog, '-c', 'case_badbmp.yaml', 'FOO')
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("Unsupported image format"))


# Run these tests
if __name__ == '__main__':
  varname = 'BMPBLK'
  if varname not in os.environ:
    print('You must specify the path to bmpblk_utility in the $%s '
          'environment variable.' % varname)
    sys.exit(1)
  prog = os.environ[varname]
  print "Testing prog...", prog
  unittest.main()

