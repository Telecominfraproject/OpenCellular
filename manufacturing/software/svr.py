#!/usr/bin/python3
# -*- coding: utf-8 -*-

# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

#
#	File: svr.py
#	
#	This is a socket server that will launch the /usr/local/fbin/fbt
#	program and communicate with it through a UNIX socket.
#
import tkinter as tk
from tkinter import *
from mods.tk import mk_tk

if __name__ == "__main__":
	tkwin = mk_tk()
	tkwin.run_gui()
