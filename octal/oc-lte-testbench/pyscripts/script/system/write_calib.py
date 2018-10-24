import sys, os
from opentest.script import testrunner, utils
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

import time

class WriteCalibration(testrunner.TestSuite):

    @testrunner.testcase("Write Calibration Files")
    def SY_CLN040(self, context):
        lte_calib.write_calib(context)
