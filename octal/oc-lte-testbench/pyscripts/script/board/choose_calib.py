from nute.core import *

class ChooseCalib(TestSuite):

    @testcase('Choose calib')
    def SY_PRP041(self, context):
        if context.CALIBRATION_ACTION == 'copy':
            context.CALIBRATION_POSTPROCESS_FOLDER = context.CALIBRATION_DEFAULT_PATH
