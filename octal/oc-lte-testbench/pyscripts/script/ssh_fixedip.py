import opentest.script.testrunner as testrunner
from  tools.lte import lte_calibtools
import time

class SSHFixedIp(testrunner.TestSuite):

    @testrunner.testcase("Boot Validation", critical=True)
    def BB_TSC003(self, context):
        try:
            context.server.ssh.wait_for_initializing(context)
        except:
            context.SSH_PASSWORD = '' # Try empty password
            context.server.ssh.wait_for_initializing(context)

    #@testrunner.testcase("TEMP")
    def BB_TSC005(self, context):
        context.server.ssh.execute_command('i2cset -f -y 1 0x21 0x03 0x60')
        context.server.ssh.execute_command('i2cset -f -y 1 0x21 0x07 0x8F')
        context.server.ssh.execute_command('i2cset -f -y 1 0x21 0x03 0x70')

    @testrunner.testcase("Launch SCP Service", critical=True)
    def BB_PRP006(self, context):
        lte_calibtools.launchscp(context)
