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


class TestFailures(unittest.TestCase):

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

  def testBadCompression(self):
    """Wrong compression types should fail."""
    rc, out, err = runprog(prog, '-z', '99', '-c', 'case_simple.yaml', 'FOO')
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("compression type"))


class TestOverWrite(unittest.TestCase):

  def setUp(self):
    rc, out, err = runprog('/bin/rm', '-rf', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)

  def testOverwrite(self):
    """Create, unpack, unpack again, with and without -f"""
    rc, out, err = runprog(prog, '-c', 'case_simple.yaml', 'FOO')
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', './FOO_DIR', 'FOO')
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("File exists"))
    rc, out, err = runprog(prog, '-x', '-d', './FOO_DIR', '-f', 'FOO')
    self.assertEqual(0, rc)

  def tearDown(self):
    rc, out, err = runprog('/bin/rm', '-rf', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)


class TestPackUnpack(unittest.TestCase):

  def setUp(self):
    rc, out, err = runprog('/bin/rm', '-rf', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)

  def testPackUnpack(self):
    """Create, unpack, recreate without compression"""
    rc, out, err = runprog(prog, '-c', 'case_simple.yaml', 'FOO')
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)
    os.chdir('./FOO_DIR')
    rc, out, err = runprog(prog, '-c', 'config.yaml', 'BAR')
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', '../FOO', 'BAR')
    self.assertEqual(0, rc)
    os.chdir('..')

  def doPackUnpackZ(self, comp):
    """Create, unpack, recreate with a given compression"""
    rc, out, err = runprog(prog, '-z', comp, '-c', 'case_simple.yaml', 'FOO')
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)
    os.chdir('./FOO_DIR')
    rc, out, err = runprog(prog, '-z', comp, '-c', 'config.yaml', 'BAR')
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', '../FOO', 'BAR')
    self.assertEqual(0, rc)
    os.chdir('..')

  def testPackUnpackZ1(self):
    """Create, unpack, recreate with EFIv1 compression"""
    self.doPackUnpackZ('1');

  def testPackUnpackZ2(self):
    """Create, unpack, recreate with LZMA compression"""
    self.doPackUnpackZ('2');

  def tearDown(self):
    rc, out, err = runprog('/bin/rm', '-rf', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)


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

