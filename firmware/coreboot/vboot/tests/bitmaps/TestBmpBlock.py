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
import tempfile
import unittest

def runprog(*args):
  """Runs specified program and args, returns (exitcode, stdout, stderr)."""
  p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  out, err = p.communicate()
  return (p.returncode, out, err)


class TempDirTestCase(unittest.TestCase):
  """A TestCase that sets up self.tempdir with a temporary directory."""

  def setUp(self):
    self.tempdir = tempfile.mkdtemp(prefix='tmp_test_bmp_block')
    self.tempfile = os.path.join(self.tempdir, 'FOO')
    self._cwd = os.getcwd()

  def tearDown(self):
    os.chdir(self._cwd)
    runprog('rm', '-rf', self.tempdir)


class TestFailures(TempDirTestCase):

  def testNoArgs(self):
    """Running with no args should print usage and fail."""
    rc, out, err = runprog(prog)
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("missing BMPBLOCK name"))
    self.assertTrue(out.count("bmpblk_utility"))

  def testMissingBmp(self):
    """Missing a bmp specified in the yaml is an error."""
    rc, out, err = runprog(prog, '-c', 'case_nobmp.yaml', self.tempfile)
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("No such file or directory"))

  def testInvalidBmp(self):
    """A .bmp file that isn't really a BMP should fail."""
    rc, out, err = runprog(prog, '-c', 'case_badbmp.yaml', self.tempfile)
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("Unsupported image format"))

  def testBadCompression(self):
    """Wrong compression types should fail."""
    rc, out, err = runprog(prog, '-z', '99', '-c', 'case_simple.yaml', self.tempfile)
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("compression type"))


class TestOverWrite(TempDirTestCase):

  def testOverwrite(self):
    """Create, unpack, unpack again, with and without -f"""
    rc, out, err = runprog(prog, '-c', 'case_simple.yaml', self.tempfile)
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', self.tempdir, self.tempfile)
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', self.tempdir, self.tempfile)
    self.assertNotEqual(0, rc)
    self.assertTrue(err.count("File exists"))
    rc, out, err = runprog(prog, '-x', '-d', self.tempdir, '-f', self.tempfile)
    self.assertEqual(0, rc)


class TestPackUnpack(TempDirTestCase):

  def testPackUnpack(self):
    """Create, unpack, recreate without compression"""
    foo = os.path.join(self.tempdir, 'FOO')
    bar = os.path.join(self.tempdir, 'BAR')
    rc, out, err = runprog(prog, '-c', 'case_simple.yaml', foo)
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', self.tempdir, foo)
    self.assertEqual(0, rc)
    os.chdir(self.tempdir)
    rc, out, err = runprog(prog, '-c', 'config.yaml', bar)
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', foo, bar)
    self.assertEqual(0, rc)

  def doPackUnpackZ(self, comp):
    """Create, unpack, recreate with a given compression"""
    foo = os.path.join(self.tempdir, 'FOO')
    bar = os.path.join(self.tempdir, 'BAR')
    rc, out, err = runprog(prog, '-z', comp, '-c', 'case_simple.yaml', foo)
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', self.tempdir, foo)
    self.assertEqual(0, rc)
    os.chdir(self.tempdir)
    rc, out, err = runprog(prog, '-z', comp, '-c', 'config.yaml', bar)
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', foo, bar)
    self.assertEqual(0, rc)

  def testPackUnpackZ1(self):
    """Create, unpack, recreate with EFIv1 compression"""
    self.doPackUnpackZ('1')

  def testPackUnpackZ2(self):
    """Create, unpack, recreate with LZMA compression"""
    self.doPackUnpackZ('2')

  def doPackUnpackImplicitZ(self, comp, noncomp):
    """Create with given compression, unpack, repack without specifying"""
    foo = os.path.join(self.tempdir, 'FOO')
    bar = os.path.join(self.tempdir, 'BAR')
    # create with the specified compression scheme
    rc, out, err = runprog(prog, '-z', comp, '-c', 'case_simple.yaml', foo)
    self.assertEqual(0, rc)
    # unpack. yaml file should have compression scheme in it
    rc, out, err = runprog(prog, '-f', '-x', '-d', self.tempdir, foo)
    self.assertEqual(0, rc)
    os.chdir(self.tempdir)
    # create with no compression specified, should use default from yaml
    rc, out, err = runprog(prog, '-c', 'config.yaml', bar)
    self.assertEqual(0, rc)
    # so new output should match original
    rc, out, err = runprog('/usr/bin/cmp', foo, bar)
    self.assertEqual(0, rc)
    # Now make sure that specifying a compression arg will override the default
    for mycomp in noncomp:
      # create with compression scheme different from default
      rc, out, err = runprog(prog, '-z', str(mycomp), '-c', 'config.yaml', bar)
      self.assertEqual(0, rc)
      # should be different binary
      rc, out, err = runprog('/usr/bin/cmp', foo, bar)
      self.assertNotEqual(0, rc)

  def testPackUnpackImplicitZ(self):
    """Create, unpack, recreate with implicit compression"""
    self._allowed = range(3)
    for c in self._allowed:
      os.chdir(self._cwd)
      self.doPackUnpackImplicitZ(str(c), [x for x in self._allowed if x != c])


class TestReproducable(TempDirTestCase):

  def disabledTestReproduce(self):
    """Equivalent yaml files should produce identical bmpblocks"""
    # TODO: This test is currently broken because bmpblock_utility
    # uses a map to hold the images, and the map doesn't preserve image
    # order.  So a simple compare is insufficient to determine that
    # the bmpblocks are equivalent.  See crosbug.com/19541.
    order1 = os.path.join(self.tempdir, 'ORDER1')
    order2 = os.path.join(self.tempdir, 'ORDER2')
    rc, out, err = runprog(prog, '-c', 'case_order1.yaml', order1)
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-c', 'case_order2.yaml', order2)
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', order1, order2)
    self.assertEqual(0, rc)


class TestReuse(TempDirTestCase):

  def testReuse(self):
    """Reusing screens in the yaml file should be okay"""
    foo = os.path.join(self.tempdir, 'FOO')
    bar = os.path.join(self.tempdir, 'BAR')
    rc, out, err = runprog(prog, '-c', 'case_reuse.yaml', foo)
    self.assertEqual(0, rc)
    rc, out, err = runprog(prog, '-x', '-d', self.tempdir, foo)
    self.assertEqual(0, rc)
    os.chdir(self.tempdir)
    rc, out, err = runprog(prog, '-c', 'config.yaml', bar)
    self.assertEqual(0, rc)
    rc, out, err = runprog('/usr/bin/cmp', foo, bar)
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
