# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os, re
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

def dict_from_command(command):
    dict = {}
    out = os.popen(command)
    for linecr in out.readlines():
        line = linecr.strip()
        match = re.match("([^ ]+) (.*)", line)
        k = match.group(1)
        v = match.group(2)
        dict[k] = v
    return dict

def expect(d, key, value):
    if (d[key] != value):
        utils.system("start tcsd", ignore_status=True)
        raise error.TestError("expecting %s = %s, receiving %s = %s" %
                              (key, value, key, d[key]))

class hardware_TPMCheck(test.test):
    version = 1

    def run_once(self):
        utils.system("stop tcsd", ignore_status=True)

        d = dict_from_command("tpmc getvf");
        expect(d, "deactivated", "0")
        expect(d, "physicalPresence", "0")
        expect(d, "physicalPresenceLock", "1")
        expect(d, "bGlobalLock", "1")

        d = dict_from_command("tpmc getpf");
        expect(d, "disable", "0")
        expect(d, "ownership", "1")
        expect(d, "deactivated", "0")
        expect(d, "physicalPresenceHWEnable", "0")
        expect(d, "physicalPresenceCMDEnable", "1")
        expect(d, "physicalPresenceLifetimeLock", "1")
        expect(d, "nvLocked", "1")

        utils.system("start tcsd", ignore_status=True)
