import sys, os
import opentest
from opentest.script import testrunner, utils
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

import time

class RFPALCalibration(testrunner.TestSuite):

    @testrunner.testcase("Calibrate RFPAL TX1")
    def FE_TSC120(self, context):
        lte_calib.calibrate_rfpal(context, tx=0, freq=context.RF_DL_MIDDLE_FREQ)

    @testrunner.testcase("Calibrate RFPAL TX2")
    def FE_TSC220(self, context):
        lte_calib.calibrate_rfpal(context, tx=1, freq=context.RF_DL_MIDDLE_FREQ)
