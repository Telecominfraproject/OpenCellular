# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import shutil
import sys
from autotest_lib.server import test, autotest
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error

class hardware_TPMFirmwareServer(test.test):
    """
    Test of TPM functionality needed in firmware (server side of the test).
    See also client/site_tests/hardware_TPMFirmware.  The server side of the
    test is used to coordinate the multiple reboots needed to bring the TPM to
    a new state (for instance between owned and unowned).
    """
    version = 1
    n_client_reboots = 0
    client_at = None

    # Run the client subtest named [subtest].
    def tpm_run(self, subtest, ignore_status=False):
        self.client_at.run_test(self.client_test, subtest=subtest)
        cstatus = self.job.get_state("client_status")
        logging.info("server: client status = %s", cstatus)
        self.job.set_state("client_status", None)
        if not ignore_status and cstatus != 0:
            error.TestFail("client subtest %s failed with status %s" %
                           (subtest, cstatus))
        return cstatus


    def reboot_client(self):
        # Reboot the client
        logging.info('TPMFirmwareServer: rebooting %s number %d' %
                     (self.client.hostname, self.n_client_reboots))
        self.client.reboot()
        self.n_client_reboots += 1


    def run_once(self, host=None):
        self.client = host
        self.client_at = autotest.Autotest(self.client)
        self.client_test = 'hardware_TPMFirmware'

        self.job.set_state("client_status", None)

        # Set up the client in the unowned state.
        self.reboot_client()
        self.tpm_run("tpmtest_clear", ignore_status=True)

        self.reboot_client()
        self.tpm_run("tpmtest_enable", ignore_status=True)

        self.reboot_client()
        self.tpm_run("tpmtest_readonly")

        self.reboot_client()
        self.tpm_run("tpmtest_globallock")

        self.reboot_client()
        self.tpm_run("takeownership")

        self.reboot_client()
        self.tpm_run("tpmtest_readonly")

        self.reboot_client()
        self.tpm_run("tpmtest_globallock")
