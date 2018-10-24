import opentest
#from opentest.script import testrunner
from tools.lte import lte_calib

class OCXOCalibration():
# class OCXOCalibration(testrunner.TestSuite):

    # @testrunner.testcase("OCXO Calibration")
    def BB_TSC020(self, context):
        lte_calib.calibrate_ocxo(context, tx=0, freq=context.RF_DL_MIDDLE_FREQ)
