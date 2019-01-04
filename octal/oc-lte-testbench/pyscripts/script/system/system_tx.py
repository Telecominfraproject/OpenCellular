import sys, os
#from nute.core import CriteriaDeclaration
import opentest
#from opentest.script import testrunner, \
from nute import utils
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

import time

# FIXME this is needed to do validation
# calibration_criteria = testrunner.CriteriaDeclaration(
#     POWER_OUT_dBm= 'CALIBRATION_VALID_MIN_POWER < value < CALIBRATION_VALID_MAX_POWER',
#     PA_CURRENT= 'value < CALIBRATION_VALID_CURRENT',
#     ACLR= 'value < CALIBRATION_VALID_ACLR',
#     FREQ= 'value',
#     FREQ_ERROR= 'value < CALIBRATION_OCXO_MAX_FREQ_ERROR',
#     EVM= 'value < CALIBRATION_VALID_EVM'
# )

# class SystemCalibPreparation(testrunner.TestSuite):
class SystemCalibPreparation():

    #@testrunner.testcase("Set Auto Spectrum Attenuation")
    def SY_PRP010(self, context):
        #lte_calib.inspect_rolloff(context, 1, 10, context.CALIBRATION_BW10_ALL_FREQS)
        context.server.spectrum.setup_attenuation(mech_atten=10)

class SystemCalib20MHz():
# class SystemCalib20MHz(testrunner.TestSuite):

    # @testrunner.testcase("TX2 20 MHz", critical=True)
    def SY_TSC230(self, context):
        lte_calib.calibrate_tx(context, 1, 20)

    # @testrunner.testcase("TX1 20 MHz", critical=True)
    def SY_TSC130(self, context):
        lte_calib.calibrate_tx(context, 0, 20)

    # @testrunner.declare_criteria(calibration_criteria)
    # @testrunner.testcase("Validate Calib 20 MHz")
    def SY_TSCX31(self, context):
        bw= 20
        lte_calib.generate_postprocess_tables(context, bw)
        lte_calib.create_cal_tgz_to_remote(context, '/tmp/')
        lte_calib.validate_eeprom_calibration_both_tx_at_BMT_2_rand(context, bw, cal_tgz_path='/tmp/')

    #@testrunner.testcase("Sweep complete table 20 MHz")
    def SY_TSCX32(self, context):
        bw= 20
        lte_calib.sweep_attens_folder(context, 0, bw, range(1815, 1870, 5), cal_tgz_path='/tmp/')
        lte_calib.sweep_attens_folder(context, 1, bw, range(1815, 1870, 5), cal_tgz_path='/tmp/')

# class SystemCalib10MHz(testrunner.TestSuite):
class SystemCalib10MHz():

    # @testrunner.testcase("TX2 10 MHz", critical=True)
    def SY_TSC210(self, context):
        #lte_calib.inspect_rolloff(context, 1, 10, context.CALIBRATION_BW10_ALL_FREQS)
        lte_calib.calibrate_tx(context, 1, 10)

    # @testrunner.testcase("TX1 10 MHz", critical=True)
    def SY_TSC110(self, context):
        #lte_calib.inspect_rolloff(context, 0, 10, context.CALIBRATION_BW10_ALL_FREQS)
        lte_calib.calibrate_tx(context, 0, 10)

    def SY_TSC115(self, context):
        #lte_calib.inspect_rolloff(context, 0, 10, context.CALIBRATION_BW10_ALL_FREQS)
        lte_calib.calibrate_tx_b5(context, 0, 10)

    def SY_TSC128(self, context):
        #lte_calib.inspect_rolloff(context, 0, 10, context.CALIBRATION_BW10_ALL_FREQS)
        #ant1
        lte_calib.calibrate_tx_b28(context, 0, 10)
        #ant2
        #lte_calib.calibrate_tx_b28(context, 1, 10)
        se = 'n'
        while (se != 'y'):
            se = raw_input("Are you ready to switch to ant2?(y):")
            if (se == 'y') or (se == 'Y'):
                se = 'y'
                print("\nNow testing ant2")
                break
        lte_calib.calibrate_tx_b28(context, 1, 10)
    def SY_TSC128_2(self, context):
        #lte_calib.inspect_rolloff(context, 0, 10, context.CALIBRATION_BW10_ALL_FREQS)
        #ant1
        lte_calib.calibrate_tx_b28(context, 1, 10)

    # @testrunner.declare_criteria(calibration_criteria)
    # @testrunner.testcase("Validate Calib 10 MHz")
    def SY_TSCX11(self, context):
        bw= 10
        lte_calib.generate_postprocess_tables(context, bw)
        #FIXME for now ignore these steps below, scp the tgz doesn't work and eeprom validation not working
        lte_calib.create_cal_tgz_to_remote(context, '/tmp/')
		#FIXME splitting to validate one ant at a time and commenting out ant2
        #lte_calib.validate_eeprom_calibration_both_tx_at_BMT_2_rand(context, bw, cal_tgz_path='/tmp/')
        #lte_calib.validate_eeprom_calibration_single_tx_at_BMT_2_rand(context, bw, cal_tgz_path='/tmp/', tx=0)
        #lte_calib.validate_eeprom_calibration_single_tx_at_BMT_2_rand(context, bw, cal_tgz_path='/tmp/', 1)

    #@testrunner.testcase("Sweep complete table 10 MHz")
    def SY_TSCX12(self, context):
        bw= 10
        lte_calib.sweep_attens_folder(context, 0, bw, range(1810, 1876, 5), cal_tgz_path='/tmp/')
        lte_calib.sweep_attens_folder(context, 1, bw, range(1810, 1876, 5), cal_tgz_path='/tmp/')

# class SystemCalib5MHz(testrunner.TestSuite):
class SystemCalib5MHz():

    # @testrunner.testcase("TX2 5 MHz", critical=True)
    def SY_TSC220(self, context):
        lte_calib.calibrate_tx(context, 1, 5)

    # @testrunner.testcase("TX1 5 MHz", critical=True)
    def SY_TSC120(self, context):
        lte_calib.calibrate_tx(context, 0, 5)

    # @testrunner.declare_criteria(calibration_criteria)
    # @testrunner.testcase("Validate Calib 5 MHz")
    def SY_TSCX21(self, context):
        bw= 5
        lte_calib.generate_postprocess_tables(context, bw)
        lte_calib.create_cal_tgz_to_remote(context, '/tmp/')
        lte_calib.validate_eeprom_calibration_both_tx_at_BMT_2_rand(context, bw, cal_tgz_path='/tmp/')

    #@testrunner.testcase("Sweep complete table 5 MHz")
    def SY_TSCX22(self, context):
        bw= 5
        lte_calib.sweep_attens_folder(context, 0, bw, range(1808, 1878, 5), cal_tgz_path='/tmp/')
        lte_calib.sweep_attens_folder(context, 1, bw, range(1808, 1878, 5), cal_tgz_path='/tmp/')

    #@testrunner.testcase("PAUSE", critical=True)
    def SY_TSCX13(self, context):
        context.wait_tester_feedback('5 MHz done')

# class SystemCalibCleanup(testrunner.TestSuite):
class SystemCalibCleanup():

    # @testrunner.testcase("Warmed-up OCXO Calibration")
    def SY_TSC035(self, context):
        lte_calib.calibrate_ocxo(context, tx=1, freq=context.RF_DL_MIDDLE_FREQ, force_calib=True)

    #@testrunner.testcase("Set Default Spectrum Attenuation")
    def SY_CLN010(self, context):
        #lte_calib.inspect_rolloff(context, 1, 10, context.CALIBRATION_BW10_ALL_FREQS)
        context.server.spectrum.setup_attenuation()
