#!/usr/bin/python -tt
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Display for bmpblock object."""
import wx

class MyPanel(wx.Panel):

  def __init__(self, parent):
    wx.Panel.__init__(self, parent, wx.ID_ANY)
    self.Bind(wx.EVT_PAINT, self.OnPaint)
    self.parent = parent
    self.imglist = ()

  def OnPaint(self, evt=None):
    if (evt):
      dc = wx.PaintDC(self)
    else:
      dc = wx.ClientDC(self)

    done_first = False
    # The first image in the sequence may be used by the BIOS to set the
    # display resolution. Regardless, it should match the desired or default
    # resolution so that any previous screens get cleared.
    for x, y, filename in self.imglist:
      img = wx.Image(filename, wx.BITMAP_TYPE_ANY)
      if (not done_first):
        size = img.GetSize()
        self.SetMinSize(size)
        self.SetSize(size)
        self.Fit()
        w,h = self.parent.GetBestSize()
        self.parent.SetDimensions(-1, -1, w, h, wx.SIZE_AUTO)
        done_first = True
      bmp = img.ConvertToBitmap()
      dc.DrawBitmap(bmp, x, y)

  def OnSave(self, name):
    """Draw the current image sequence into a file."""
    dc = wx.MemoryDC()
    done_first = False
    for x, y, filename in self.imglist:
      img = wx.Image(filename, wx.BITMAP_TYPE_ANY)
      if (not done_first):
        w,h = img.GetSize()
        base = wx.EmptyBitmap(w,h)
        dc.SelectObject(base)
        done_first = True
      bmp = img.ConvertToBitmap()
      dc.DrawBitmap(bmp, x, y)
    new = wx.ImageFromBitmap(base)
    outfile = name + '.png'
    new.SaveFile(outfile, wx.BITMAP_TYPE_PNG)
    print "wrote", outfile


class Frame(wx.Frame):

  def __init__(self, bmpblock=None, title=None):
    wx.Frame.__init__(self, None, wx.ID_ANY, title=title)
    self.CreateStatusBar()
    self.SetStatusText(title)
    self.Bind(wx.EVT_CLOSE, self.OnQuit)

    self.bmpblock = bmpblock
    if self.bmpblock:
      self.bmpblock.RegisterScreenDisplayObject(self)

    self.p = MyPanel(self)


  def OnQuit(self, event):
    wx.GetApp().ExitMainLoop()

  def DisplayScreen(self, name, imglist):
    self.SetStatusText(name)
    self.p.imglist = imglist
    self.p.OnPaint()

  def SaveScreen(self, name, imglist):
    self.p.imglist = imglist
    self.p.OnSave(name)
