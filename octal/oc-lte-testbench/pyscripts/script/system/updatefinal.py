import sys, os
import opentest
import opentest.script.testrunner as testrunner
from opentest.interface import filexfer
from tools.lte import lte_calibtools

class UpdateFinal(testrunner.TestSuite):

    @testrunner.testcase("Update final env from ssh", critical=True)
    def CM_CLN003(self, context):
        context.logger.info("Setup final env in progress")
        env_file_path = os.path.join(context.TEST_PACKAGE_PATH, context.UBOOT_FINALFILEENV);
        context.server.env_var.setup_from_file(env_file_path)

    @testrunner.testcase("Validate boot from flash")
    def SY_TSC050(self, context):
        context.server.tftp.kill()#Closes the TFTP server
        context.server.ssh.reboot()
        context.server.ssh.wait_for_initializing(context)
