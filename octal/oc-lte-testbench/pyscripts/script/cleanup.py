from opentest.script.testrunner import TestSuite,testcase
from tools.lte import lte_calibtools
import time
import os

class Cleanup(TestSuite):

    @testcase("Set TX paths off")
    def FE_CLN001(self, context):
        try:
            lte_calibtools.set_tx_enable(context, 0, 0)
            lte_calibtools.set_tx_enable(context, 1, 0)
            context.logger.info("TXs are off")
        except:
            context.logger.warning("Failed to set TXs off")


    @testcase("Power off")
    def BB_CLN001(self, context):
        try:
            context.server.dcsupply.setenable(0)
        except:
            context.logger.warning("Failed to set power off")

    #@testcase("Open test logs")
    def OpenLogs(self, context):
        import opentest.script.log_scripts as log_scripts
        context.wait_tester_feedback("Open test logs ?")
        log_scripts.open_logs(context)
