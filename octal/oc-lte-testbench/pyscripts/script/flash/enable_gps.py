import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

class EnableGPS(testrunner.TestSuite):

    @testrunner.testcase("Enable GPS")
    def BB_CLN003(self, context):
        context.logger.info("Enabling GPS")
        context.server.env_var.set_value('gpsenable', 1)
        context.server.env_var.set_value('syncby', 'gps')
