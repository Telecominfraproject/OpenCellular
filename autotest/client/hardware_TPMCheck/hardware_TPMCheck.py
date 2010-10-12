# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

def old_or_missing_firmware_version():
    f = open("/sys/devices/platform/chromeos_acpi/FWID")
    if not f:
        return True
    version = f.readline().strip()
    logging.info("firmware version: %s", version)
    # Expect a dot-separated list of 6 elements.  Discard 1st element.
    v = re.split("\.", version)[1:]
    w = re.split("\.", "any-nickname.03.60.1118.0036.")[1:]
    if len(v) != len(w):
        raise error.TestError("malformed firmware version %s" % version)
    return v < w

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
        raise error.TestError("expecting %s = %s, observing %s = %s" %
                              (key, value, key, d[key]))

def checkp(space, permission):
    c = "tpmc getp %s" % space
    l = os.popen(c).readline()
    if (not re.match(".*%s" % permission, l)):
        raise error.TestError("invalid response to %s: %s" % (c, l))

class hardware_TPMCheck(test.test):
    version = 1

    def run_once(self):

        if old_or_missing_firmware_version():
            logging.warning("skipping test because firmware " +
                            "version missing or deemed too old")
            return

        try:
            utils.system("stop tcsd", ignore_status=True)

            # Check volatile (ST_CLEAR) flags
            d = dict_from_command("tpmc getvf");
            expect(d, "deactivated", "0")
            expect(d, "physicalPresence", "0")
            expect(d, "physicalPresenceLock", "1")
            expect(d, "bGlobalLock", "1")

            # Check permanent flags
            d = dict_from_command("tpmc getpf");
            expect(d, "disable", "0")
            expect(d, "ownership", "1")
            expect(d, "deactivated", "0")
            expect(d, "physicalPresenceHWEnable", "0")
            expect(d, "physicalPresenceCMDEnable", "1")
            expect(d, "physicalPresenceLifetimeLock", "1")
            expect(d, "nvLocked", "1")

            # Check space permissions
            checkp("0x1007", "0x8001")
            checkp("0x1008", "0x1")

            # Check kernel space UID
            l = os.popen("tpmc read 0x1008 0x5").readline()
            if (not re.match(".* 4c 57 52 47$", l)):
                raise error.TestError("invalid kernel space UID: %s" % l)

        finally:
            utils.system("start tcsd", ignore_status=True)
