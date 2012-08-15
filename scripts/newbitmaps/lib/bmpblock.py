#!/usr/bin/python -tt
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A BmpBlock class"""

import os
import types
import yaml

class BmpBlock(object):
  """A wrapper for the config.yaml file.
  It has a few special attributes to specify which part we're focusing on.
  """

  def __init__(self, libdir, filename=None):
    self.yaml = None
    self.filename = None
    self.current_screen = None
    self.libdir = libdir
    self.filename = filename                    # always set, so we can reload
    if filename:
      self.LoadFile(filename)

  def LoadFile(self, filename):
    """Load the specified yaml file and verify that it's a valid BmpBlock"""
    print "Loading", filename
    with open(filename, 'rb') as f:
      stuff = yaml.safe_load(f)
    # FIXME: This is pretty lame. We should be able to find images using a
    # default directory path instead of using chdir.
    if os.path.dirname(filename):
      os.chdir(os.path.dirname(filename))
    if self.IsValidSyntax(stuff):
      self.yaml = stuff
      self.current_screen = sorted(self.yaml["screens"].keys())[0]

  def Reload(self):
    tmp = self.current_screen
    self.LoadFile(self.filename)
    if tmp in self.yaml["screens"]:
      self.current_screen = tmp

  def IsValidSyntax(self, thing):
    """Raise an error if the specified dict is not a valid BmpBlock structure"""

    assert isinstance(thing, dict)

    seen_images = {"$HWID":1, "$HWID.rtol":2}
    seen_screens = {}

    images = thing["images"]
    assert isinstance(images, dict)
    assert len(images) > 0
    # image values should all be filenames (ie, strings)
    for val in images.values():
      assert val and isinstance(val, types.StringTypes)
    # don't worry about fonts. eventually we'll have graphical mocks on host.
    if "$HWID" in images:
      print "WARNING: ignoring $HWID font blob"
    if "$HWID.rtol" in images:
      print "WARNING: ignoring $HWID.rtol font blob"
    # TODO(hungte) Replace this by rendering with font block.
    images["$HWID"] = 'hwid_placeholder.bmp'
    images["$HWID.rtol"] = 'hwid_placeholder.bmp'

    screens = thing["screens"]
    assert isinstance(screens, dict)
    assert screens
    # screen values should all be lists of 3-tuples
    for scrname, imglist in screens.items():
      assert len(imglist) <= 16
      for img in imglist:
        assert 3 == len(img)
        # must have defined all referenced bitmaps
        x,y,i = img
        assert i in images
        seen_images[i] = True

    localizations = thing["localizations"]
    assert hasattr(localizations, '__iter__')
    assert localizations
    # localizations should all be lists with the same number of screens
    len0 = len(localizations[0])
    assert len0
    for elt in localizations:
      assert len0 == len(elt)
      # we must have defined all referenced screens
      for scr in elt:
        assert scr in screens
        seen_screens[scr] = True

    for unused_img in [x for x in images if x not in seen_images]:
      print "  Unused image:", unused_img
    for unused_scr in [x for x in screens if x not in seen_screens]:
      print "  Unused screen:", unused_scr

    return True

  def RegisterScreenDisplayObject(self, displayer):
    """Register an object with a .Redisplay() function to display updates."""
    self.displayer = displayer


  def Redisplay(self):
    """Redisplay contents."""
    if self.displayer:
      if self.current_screen:
        sc = self.yaml['screens'][self.current_screen]
        slist = [(x,y,self.yaml['images'][z]) for x,y,z in sc]
        self.displayer.DisplayScreen(self.current_screen, slist)

  def Saveit(self):
    """Save current screen to file."""
    if self.displayer:
      if self.current_screen:
        sc = self.yaml['screens'][self.current_screen]
        slist = [(x,y,self.yaml['images'][z]) for x,y,z in sc]
        self.displayer.SaveScreen(self.current_screen, slist)
