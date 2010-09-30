# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os, re, sys, shutil
from autotest_lib.client.bin import test, utils

class hardware_TPMFirmware(test.test):
    """
    Test of TPM functionality needed in firmware (client side of the test).
    See also server/site_tests/hardware_TPMFirmwareServer.
    """
    version = 1
    preserve_srcdir = True

    def setup(self):
        utils.make('-C %s' % self.srcdir)

    # Runs a command, logs the output, and returns the exit status.
    def tpm_run(self, cmd, ignore_status=False):
        output = utils.run(cmd, ignore_status=ignore_status)
        logging.info(output)
        self.job.set_state("client_status", output.exit_status)


    def run_once(self, subtest='None'):
        logging.info("Running TPM firmware client subtest %s", subtest)
        if (subtest == 'takeownership'):
            output = utils.run("start tcsd", ignore_status=False)
            # When TCSD is running, the system might try to take ownership as
            # well.  We don't care.
            logging.info(output)
            own_cmd = "tpm_takeownership -y -z"
            self.tpm_run(own_cmd, ignore_status=True)
        else:
            cmd = os.path.join(self.srcdir, subtest)
            self.tpm_run(cmd, ignore_status=True)
