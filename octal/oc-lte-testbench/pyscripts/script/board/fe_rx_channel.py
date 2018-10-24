import sys
import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools
from opentest.integration import rfmeasure
import time


##region RXAttenuatorValidation

class SetupRXSwitch(testrunner.TestSuite):

    @testrunner.testcase("Set RX Sniffer switch")
    def FE_PRP040(self, context):
        lte_calibtools.set_rx1_switches(context)

    @testrunner.testcase("Turn off TX")
    def FE_PRP041(self, context):
        lte_calibtools.set_all_tx_off(context)


class RXAttenuatorValidation(testrunner.TestSuite):

    @testrunner.testcase("Validate RX 1 Attenuator bits")
    def FE_TSC040(self, context):
        validate_rx_attn(context, 0)

    @testrunner.testcase("Validate RX 2 Attenuator bits")
    def FE_TSC140(self, context):
        validate_rx_attn(context, 1)

def validate_rx_attn(context, tx):
    spectrum = context.server.spectrum

    def set_attn_fn(value):
        lte_calibtools.set_rx_atten_raw(context, context.server.ssh, tx, value)

    def read_power_fn():
        freq, pwr1 = spectrum.find_peak_stable_amp(stable_diff=0.5)
        pwr2 = spectrum.get_peak_amp()
        return (pwr1+pwr2)/2.0

    component_tester = context.server.component_tester
    rfmeasure.set_spectrum_path(context, 'UUT_RX', tx)
    rfmeasure.set_generator_path(context, 'UUT_ANT', tx)

    component_tester.setup_tone()
    component_tester.setup_tone_measure(center_freq_mhz=context.RF_UL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_ANT_INPUT_POWER_dBm)
    lte_calibtools.set_rx_enable(context, tx, 1)

    time.sleep(context.RF_STABILISATION_TIME*2)

    lte_calibtools.generic_valid_attn(context, set_attn_fn=set_attn_fn, power_fn=read_power_fn, criteria_prefix='FE_TSC%d40_' % lte_calibtools.real_tx(tx))

##endregion

##region LNA Gain

class RXEnableValidation(testrunner.TestSuite):

    @testrunner.testcase("Validate RX1 LNA Gain")
    def FE_TSC041(self, context):
        validate_rx_gain(context, 0)

    @testrunner.testcase("Validate RX2 LNA Gain")
    def FE_TSC141(self, context):
        validate_rx_gain(context, 1)


def validate_rx_gain(context, tx):

    lte_calibtools.set_rx_atten(context, context.server.ssh, tx, context.CALIBRATION_RX_ATTN_INIT_ATTEN)

    component_tester = context.server.component_tester
    rfmeasure.set_generator_path(context, 'UUT_ANT', tx)
    rfmeasure.set_spectrum_path(context, 'UUT_RX', tx)

    component_tester.setup_tone()
    component_tester.setup_tone_measure(center_freq_mhz=context.RF_UL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_ANT_INPUT_POWER_dBm)
    lte_calibtools.set_rx_enable(context, tx, 1)
    lte_calibtools.set_rx_atten(context, context.server.ssh, tx, context.CALIBRATION_RX_ATTN_INIT_ATTEN)
    time.sleep(1)
    freq, rx_power = context.server.spectrum.find_peak_stable_amp(stable_diff=0.5)
    context.logger.debug('Input power is %.2f, Power at RX is %.2f' % (context.RF_FE_ANT_INPUT_POWER_dBm, rx_power))

    lna_on_off_gain = measure_rx_on_off_gain(context, tx)
    lna_gain = rx_power-context.RF_FE_ANT_INPUT_POWER_dBm

    with context.criteria.evaluate_block() as eval_block:
        eval_block.evaluate('FE_TSC%d42_ON_OFF_LNA_GAIN_dB' % lte_calibtools.real_tx(tx), lna_on_off_gain)
        eval_block.evaluate('FE_TSC%d42_LNA_GAIN_dB' % lte_calibtools.real_tx(tx), lna_gain)
        setattr(context, 'CALIBRATION_RX%d_GAIN' % tx, lna_gain)

    lte_calibtools.set_rx_enable(context, tx, 0)

def measure_rx_on_off_gain(context, tx):
    #start with on
    freq, peak_on = context.server.spectrum.find_peak_stable_amp(stable_diff=0.5)
    lte_calibtools.set_rx_enable(context, tx, 0) #
    time.sleep(0.3)
    peak_off = context.server.spectrum.get_peak_stable_amp(stable_diff=0.5)

    power_diff = peak_on - peak_off
    return power_diff

class RXRMSDetectorValidation(testrunner.TestSuite):

    rev_rms_criteria = testrunner.CriteriaDeclaration(
        REV_RMS= 'value > 80'
    )

    #@testrunner.declare_criteria(rev_rms_criteria)
    #@testrunner.testcase("Validate TX1 RMS Detector REV")
    def FE_TSC142(self, context):
        measure_rms_rev_gain(context, 0)

    #@testrunner.declare_criteria(rev_rms_criteria)
    #@testrunner.testcase("Validate TX2 RMS Detector REV")
    def FE_TSC242(self, context):
        measure_rms_rev_gain(context, 1)

def measure_rms_rev_gain(context, tx):

    component_tester = context.server.component_tester
    rfmeasure.set_generator_path(context, 'UUT_ANT', tx)
    rfmeasure.set_spectrum_path(context, 'UUT_RX', tx)

    component_tester.setup_tone()
    component_tester.setup_tone_measure(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_ANT_INPUT_POWER_dBm)

    rms_value = float(context.server.ssh_rms.read_rms('trx%d_rev' % tx))
    context.criteria.REV_RMS(rms_value)

##endregion
