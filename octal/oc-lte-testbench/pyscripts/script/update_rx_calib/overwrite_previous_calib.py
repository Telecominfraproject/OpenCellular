import os
from nute.core import *

class OverwriteCalib(TestSuite):

    @testcase("Overwrite Calib Destination")
    def SY_TSC261(self, context):
        context.CALIBRATION_POSTPROCESS_FOLDER = os.path.join(context.PREVIOUS_CALIB_TEST_PATH, 'calib')
