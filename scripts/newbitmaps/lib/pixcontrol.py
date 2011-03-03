#!/usr/bin/python -tt
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Edit buttons for bmpblock object"""

import wx

class Frame(wx.Frame):

  def __init__(self, bmpblock=None, title=None):
    wx.Frame.__init__(self, None, wx.ID_ANY, title, size=(200,400))
    menuFile = wx.Menu()
    m_about = menuFile.Append(wx.ID_ANY, "About...\tCtrl+A")
    menuFile.AppendSeparator()
    m_reload = menuFile.Append(wx.ID_ANY, "Reload\tCtrl+R")
    m_snapshot = menuFile.Append(wx.ID_ANY, "Save snapshot")
    m_quit = menuFile.Append(wx.ID_ANY, "Quit\tCtrl+Q")
    menuBar = wx.MenuBar()
    menuBar.Append(menuFile, "&File")
    self.SetMenuBar(menuBar)
    self.CreateStatusBar()
    self.Bind(wx.EVT_MENU, self.OnAbout, m_about)
    self.Bind(wx.EVT_MENU, self.OnReload, m_reload)
    self.Bind(wx.EVT_MENU, self.OnSaveit, m_snapshot)
    self.Bind(wx.EVT_MENU, self.OnQuit, m_quit)
    self.Bind(wx.EVT_CLOSE, self.OnQuit)

    acctbl = wx.AcceleratorTable([
      (wx.ACCEL_CTRL, ord('A'), m_about.GetId()),
      (wx.ACCEL_CTRL, ord('R'), m_reload.GetId()),
      (wx.ACCEL_CTRL, ord('Q'), m_quit.GetId())
      ])

    self.SetAcceleratorTable(acctbl)

    # create UI components
    panel = wx.Panel(self)
    button_reload = wx.Button(panel, label="Reload File")
    self.screenlist = wx.ListBox(panel, wx.ID_ANY)

    # connect events
    self.Bind(wx.EVT_BUTTON, self.OnReload, button_reload)
    self.Bind(wx.EVT_LISTBOX, self.OnSelected, self.screenlist)
    self.Bind(wx.EVT_IDLE, self.OnIdle)

    # place the componenents
    sizer = wx.BoxSizer(wx.VERTICAL)
    sizer.Add(button_reload)
    sizer.Add(wx.StaticText(panel, wx.ID_ANY, "Screens"))
    sizer.Add(self.screenlist, 1, wx.EXPAND)

    panel.SetSizer(sizer)
    panel.Fit()

    # now, what are we looking at?
    self.bmpblock = bmpblock
    self.UpdateControls()
    self.do_update = True
    self.screenlist.SetFocus()

  def OnAbout(self, event):
    """Display basic information about this application."""
    msg = ("Yes, all this does right now is display the screens from the config"
           " file. You still have to edit, save, and reload in order to see any"
           " changes. Learning python and wxpython is my 20% project (actually"
           " it's more like 5%). Feel free to improve things.\n\t-- bill")
    wx.MessageBox(msg, "About", wx.OK | wx.ICON_INFORMATION, self)

  def OnQuit(self, event):
    """Close all application windows and quit."""
    wx.GetApp().ExitMainLoop()

  def OnReload(self, event):
    """Tell the model object to refresh the view that the user sees.
    FIXME: The model itself should know to do this without being told.
    """
    self.bmpblock.Reload()
    self.do_update = True;
    self.UpdateControls()

  def OnSaveit(self, event):
    """Tell the model object to save the view that the user sees."""
    self.bmpblock.Saveit()

  def OnSelected(self, event):
    """User may have picked one of the pulldowns."""
    if event.IsSelection():
      self.bmpblock.current_screen = event.GetString()
      self.do_update = True
    event.Skip()

  def UpdateControls(self):
    """Reload all the buttons with the current model information."""
    screens = self.bmpblock.yaml["screens"]
    self.screenlist.Clear()
    self.screenlist.AppendItems(sorted(screens.keys()))
    current = self.bmpblock.current_screen
    self.screenlist.SetStringSelection(current)
    self.SetStatusText(self.bmpblock.filename)

  def OnIdle(self, event=None):
    """What to do, what to do..."""
    if self.do_update:
      # FIXME: The model should know when to do this itself, right?
      self.bmpblock.Redisplay()
      self.do_update = False
