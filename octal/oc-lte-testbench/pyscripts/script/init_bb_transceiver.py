import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
import time

class BBPreparation(testrunner.TestSuite):

    @testrunner.testcase("Enable BB Transceiver", critical=True)
    def BB_PRP010(self, context):
        lte_calib.set_default_atten_values(context, 0)
        lte_calib.set_default_atten_values(context, 1)
        lte_calibtools.start_etm(context, '1.1')
        context.logger.info("Waiting for RF output...")
        time.sleep(4)
