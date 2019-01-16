import sys, os
import opentest
#from opentest.script import testrunner, utils
from nute import utils
import tools.lte.lte_calibtools as lte_calibtools
from opentest.integration import rfmeasure, calib_utils
import time




# class PrepareCalibration(testrunner.TestSuite):
class PrepareCalibration():

    #@testrunner.testcase("Temp RX tables")
    def SY_PRP040(self, context):
        for tx in range(2):
            const_name = 'CALIBRATION_BB_RX%d_GAIN' % tx
            bb_gain = getattr(context, const_name) #context.server.env_var.get_value('gain%d' % lte_calibtools.real_tx(tx))
            for bw in context.CALIBRATION_LTE_BANDWIDTHS:
                freq_range = getattr(context, 'CALIBRATION_BW%d_ALL_FREQS_UL' % bw)
                pwr_range = [30]
                temp = 30
                bb_rx_attens = [bb_gain]*(len(freq_range))
                rx_attn_value = context.CALIBRATION_RX_ATTN_EEPROM[tx]
                fe_rx_attens = [rx_attn_value*context.CALIBRATION_RX_ATTN_RESOLUTION_DIVIDER]*(len(freq_range))
                with utils.stack_chdir(context.CALIBRATION_POSTPROCESS_FOLDER):
                    context.logger.debug('Writing Temp RX Table for bw=%s and tx=%s' % (str(bw), str(tx)))
                    calib_utils.LTEFERXCalibTable().set_values(freq_range, pwr_range, temp, fe_rx_attens).save(band=context.RF_BAND, bandwidth=bw, tx=tx)
                    calib_utils.LTEBBRXCalibTable().set_values(freq_range, bb_rx_attens).save(band=context.RF_BAND, bandwidth=bw, tx=tx)

    #@testrunner.not_implemented
    #@testrunner.testcase("Calibrate RX1", critical=True)
    def SY_TSC140(self, context):
        pass

    #@testrunner.not_implemented
    #@testrunner.testcase("Calibrate RX2", critical=True)
    def SY_TSC240(self, context):
        pass
