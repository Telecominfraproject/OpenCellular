import sys, os
import opentest
import opentest.script.testrunner as testrunner
from opentest.interface import filexfer
from tools.lte import lte_calibtools

class RevertUpdateFinal(testrunner.TestSuite):

    @testrunner.testcase("Revert to TFTP if final env", critical=True)
    def SY_PRP003(self, context):
        if context.server.env_var.get_value('mode') != 'pltd':
            context.WAS_FINALENV = True
            context.logger.info("Setup env in progress")
            final_file = os.path.join(context.TEST_PACKAGE_PATH, context.UBOOT_FINALFILEENV)
            tftp_file = os.path.join(context.TEST_PACKAGE_PATH, context.UBOOT_SETUPFILEENV)
            context.server.env_var.setup_from_file_and_in_second(tftp_file, final_file)
            context.server.ssh.reboot()
            context.server.ssh.wait_for_initializing(context)
        else:
            context.WAS_FINALENV = False
