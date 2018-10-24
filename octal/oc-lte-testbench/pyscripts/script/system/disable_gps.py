import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

class TEMPDisableGPS(testrunner.TestSuite):

    @testrunner.testcase("TEMP Disable GPS")
    def BB_CLN003(self, context):
        context.logger.info("Disabling GPS (for e2e temp)")
        context.server.env_var.set_value('gpsenable', 0)
        context.server.env_var.set_value('syncby', 'internal')
