#!/usr/bin/python2.6
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Call on a directory with components_XXX_XXX files, to create
# bitmaps with the appropriate names and contents.

import sys
import glob
import os
import re
import tempfile


if '--force' in sys.argv:
  sys.argv.remove('--force')
else:
  print "This script is deprecated."
  print "The latest BIOSes can render HWIDs directly from ASCII."
  print "Use --force to proceed anyway."
  sys.exit(1)

if len(sys.argv) < 2:
  print "usage: %s --force autotest/../hardware_Components/" % sys.argv[0]
  sys.exit(1)

def MakeBmp(hwid, geom, bmp, directory):
  """ Create the bitmap for this file. """
  tmpdir = tempfile.mkdtemp()
  tmpfile = os.path.join(tmpdir, "hwid.txt")
  tmpbmpname = os.path.join(tmpdir, "hwid.bmp")
  f = open(tmpfile, "w")
  f.write(hwid)
  f.close()

  # Call bitmap making tools.
  txt_to_bmp = ("~/trunk/src/platform/vboot_reference/"
                "scripts/newbitmaps/strings/text_to_bmp")
  imagedir = os.path.join(
    "~/trunk/src/platform/vboot_reference/scripts/newbitmaps/images",  geom)
  yamlfile = os.path.join(imagedir, "unknown.yaml")
  newyamlfile = os.path.join(imagedir, "hwid.yaml")
  outputbmp = os.path.join(directory, bmp)

  os.system("%s %s > /dev/null" % (txt_to_bmp, tmpfile))
  os.system("cat %s | sed 's#hwid_unknown.bmp#%s#' > %s" % (
            yamlfile, tmpbmpname, newyamlfile))
  os.system("pushd %s >/dev/null; bmpblk_utility -c %s %s; popd >/dev/null" % (
            imagedir, newyamlfile, outputbmp))
  os.system("rm -rf %s" % tmpdir)

def ProcessDir(directory):
  """ Find all the components file in this dir. """
  # Regex to find the values we want.
  re_bmp = re.compile(r'\'data_bitmap_fv\': \[\'(?P<bmp>.*)\'\],')
  re_hwid = re.compile(r'\'part_id_hwqual\': \[\'(?P<hwid>.*)\'\],')
  re_geom = re.compile(r'\'data_display_geometry\': \[\'(?P<geom>.*)\'\],')
  # Old bitmap style
  re_fv = re.compile(r'.*\.fv')

  # Find the components files.
  files = glob.glob(os.path.join(directory, "data_*/components_*"))
  for file in files:
    # Scan for the values.
    f = open(file, "r")
    bmp = None
    hwid = None
    geom = None
    for line in f.readlines():
      m = re_bmp.search(line)
      if m:
        bmp = m.group('bmp')

      m = re_hwid.search(line)
      if m:
        hwid = m.group('hwid')

      m = re_geom.search(line)
      if m:
        geom = m.group('geom')
    f.close()
    if not ( bmp and hwid and geom):
      print "Corrupt HWID configuration"
      sys.exit(1)
    if re_fv.match(bmp):
      print "HWID: %s, %s, %s (skipping old style bitmap)" % (hwid, geom, bmp)
    else:
      print "HWID: %s, %s, %s" % (hwid, geom, bmp)
      MakeBmp(hwid, geom, bmp, directory)

def main():
  directory = os.path.abspath(sys.argv[1])
  print "Generating HWID bmp based on %s" % directory
  ProcessDir(directory)

if __name__ == '__main__':
  main()
