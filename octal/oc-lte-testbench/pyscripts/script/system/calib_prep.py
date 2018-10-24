import sys, os
import opentest
#from opentest.script import testrunner, utils
from nute import utils
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

import time


#class PrepareCalibration(testrunner.TestSuite):
class PrepareCalibration():

    #@testrunner.testcase("Create calibration folder", critical=True)
    def SY_PRP011(self, context):
        context.CALIBRATION_PREPROCESS_FOLDER = create_calib_folder(context, context.CALIBRATION_PREPROCESS_FOLDER)
        context.CALIBRATION_POSTPROCESS_FOLDER = create_calib_folder(context, context.CALIBRATION_POSTPROCESS_FOLDER)


    #@testrunner.testcase("Check Output", critical=True)
    def SY_PRP012(self, context):

        tx = 0

        lte_calibtools.enable_fe_all_on(context)
        #lte_calibtools.start_etm(context, '1.1')

        rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
        spectrum = context.server.spectrum
        ssh = context.server.ssh

        actual_bw = int(context.server.env_var.get_value('bw'))
        bw = actual_bw
        if bw != actual_bw:
            context.logger.info("Changing bandwidth; requires reboot.")
            lte_calibtools.reset_and_change_lte_bw(context, bw)

        spectrum.setup_lte_dl_mode(lte_bw=bw, cont = 'ON')

        lte_calib.setup_etm_measurement(context, context.RF_DL_MIDDLE_FREQ)

        lte_calibtools.set_tx_enable(context, tx, 1)
        lte_calib.acquire_acp(context)
        #context.wait_tester_feedback('Is there an LTE signal ?')

def create_calib_folder(context, path):
    # path = os.path.join(context.LOGS_PATH, path)
    if not os.path.exists(path):
        os.mkdir(path)
    context.logger.info("Created folder %s" % path)
    return path
