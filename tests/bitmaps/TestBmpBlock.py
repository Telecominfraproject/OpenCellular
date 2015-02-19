#!/usr/bin/python2 -tt
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
    self._cwd = os.getcwd()
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

  def doPackUnpackImplicitZ(self, comp, noncomp):
    """Create with given compression, unpack, repack without specifying"""
    # create with the specified compression scheme
    rc, out, err = runprog(prog, '-z', comp, '-c', 'case_simple.yaml', 'FOO')
    self.assertEqual(0, rc)
    # unpack. yaml file should have compression scheme in it
    rc, out, err = runprog(prog, '-f', '-x', '-d', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)
    os.chdir('./FOO_DIR')
    # create with no compression specified, should use default from yaml
    rc, out, err = runprog(prog, '-c', 'config.yaml', 'BAR')
    self.assertEqual(0, rc)
    # so new output should match original
    rc, out, err = runprog('/usr/bin/cmp', '../FOO', 'BAR')
    self.assertEqual(0, rc)
    # Now make sure that specifying a compression arg will override the default
    for mycomp in noncomp:
      # create with compression scheme different from default
      rc, out, err = runprog(prog, '-z', str(mycomp), '-c', 'config.yaml', 'BAR')
      self.assertEqual(0, rc)
      # should be different binary
      rc, out, err = runprog('/usr/bin/cmp', '../FOO', 'BAR')
      self.assertNotEqual(0, rc)
    os.chdir('..')

  def testPackUnpackImplicitZ(self):
    """Create, unpack, recreate with implicit compression"""
    self._allowed = range(3)
    for c in self._allowed:
      self.doPackUnpackImplicitZ(str(c), [x for x in self._allowed if x != c])

  def tearDown(self):
    os.chdir(self._cwd)
    rc, out, err = runprog('/bin/rm', '-rf', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)


class TestReproducable(unittest.TestCase):

  def setUp(self):
    rc, out, err = runprog('/bin/rm', '-f', 'ORDER1', 'ORDER2')
    self.assertEqual(0, rc)

  def disabledTestReproduce(self):
    """Equivalent yaml files should produce identical bmpblocks"""
    # TODO: This test is currently broken because bmpblock_utility
    # uses a map to hold the images, and the map doesn't preserve image
    # order.  So a simple compare is insufficient to determine that
    # the bmpblocks are equivalent.  See crosbug.com/19541.
    rc, out, err = runprog(prog, '-c', 'case_order1.yaml', 'ORDER1')
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-c', 'case_order2.yaml', 'ORDER2')
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', 'ORDER1', 'ORDER2')
    self.assertEqual(0, rc)

  def tearDown(self):
    rc, out, err = runprog('/bin/rm', '-f', 'ORDER1', 'ORDER2')
    self.assertEqual(0, rc)

class TestReuse(unittest.TestCase):

  def setUp(self):
    rc, out, err = runprog('/bin/rm', '-rf', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)

  def testReuse(self):
    """Reusing screens in the yaml file should be okay"""
    rc, out, err = runprog(prog, '-c', 'case_reuse.yaml', 'FOO')
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', './FOO_DIR', 'FOO')
    self.assertEqual(0, rc)
    os.chdir('./FOO_DIR')
    rc, out, err = runprog(prog, '-c', 'config.yaml', 'BAR')
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', '../FOO', 'BAR')
    self.assertEqual(0, rc)
    os.chdir('..')

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
