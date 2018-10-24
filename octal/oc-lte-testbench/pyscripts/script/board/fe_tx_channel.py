import sys
import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools
from opentest.integration import rfmeasure
import time

##region TSC034: Enables Validation

class TXEnableValidation(testrunner.TestSuite):

    @testrunner.testcase("Validate TX1 Enables Gains")
    def FE_TSC131(self, context):
        validate_tx_gains(context, 0)

    @testrunner.testcase("Validate TX2 Enables Gains")
    def FE_TSC231(self, context):
        validate_tx_gains(context, 1)


def validate_tx_gains(context, tx):

    lte_calibtools.set_tx_atten(context, context.server.ssh, tx, context.CALIBRATION_TX_ATTN_INIT_ATTEN)

    component_tester = context.server.component_tester
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    rfmeasure.set_generator_path(context, 'UUT_TX', tx)

    component_tester.setup_tone()
    component_tester.setup_tone_measure(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_TX_INPUT_POWER_dBm)

    with context.criteria.evaluate_block() as criteria_block:
    #Measure predrivers gain with PA enabled
        predr1_gain = measure_gain(context, component_tester, lte_calibtools.set_pred1, tx)
        criteria_block.evaluate('TSC%d31_PRE_DRIVER1_GAIN_dB' % lte_calibtools.real_tx(tx), predr1_gain)
        predr2_gain = measure_gain(context, component_tester, lte_calibtools.set_pred2, tx)
        criteria_block.evaluate('TSC%d31_PRE_DRIVER2_GAIN_dB' % lte_calibtools.real_tx(tx), predr2_gain)

        pa_gain = measure_gain(context, component_tester, lte_calibtools.set_pa, tx)
        criteria_block.evaluate('TSC%d31_PA_GAIN_dB' % lte_calibtools.real_tx(tx), pa_gain)

        #Set all off
        lte_calibtools.set_all_enables(context, tx, 0, 0, 0)


def measure_gain(context, component_tester, predriver_fn, tx):
    lte_calibtools.set_tx_enable(context, tx, 1)#Enable all
    time.sleep(0.3)
    peak_on = component_tester.spectrum.find_peak_stable_amp(stable_diff=0.5)
    predriver_fn(context, tx, 0)#only disable this one
    time.sleep(0.3)
    peak_off = component_tester.spectrum.find_peak_stable_amp(stable_diff=0.5)
    power_diff = peak_on[1] - peak_off[1]
    return power_diff

##endregion

##region TSC031: Attenuators Validation

class TXAttnValidation(testrunner.TestSuite):

    @testrunner.testcase("Validate TX1 Attn")
    def FE_TSC132(self, context):
        validate_tx_attn(context, 0)

    @testrunner.testcase("Validate FB1 Attn")
    def FE_TSC133(self, context):
        validate_fb_attn(context, 0)

    @testrunner.testcase("Validate TX2 Attn")
    def FE_TSC232(self, context):
        validate_tx_attn(context, 1)

    @testrunner.testcase("Validate FB2 Attn")
    def FE_TSC233(self, context):
        validate_fb_attn(context, 1)


def validate_tx_attn(context, tx):

    component_tester = context.server.component_tester

    def set_attn_fn(value):
        lte_calibtools.set_tx_atten_raw(context, context.server.ssh, tx, value)

    def read_power_fn():
        freq, pwr = component_tester.spectrum.find_peak_stable_amp(stable_diff=0.5)
        context.logger.debug("Power out = %.2f dBm" % pwr)
        return pwr

    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    rfmeasure.set_generator_path(context, 'UUT_TX', tx)

    component_tester.setup_tone()
    component_tester.setup_tone_measure(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_TX_INPUT_POWER_dBm)
    lte_calibtools.set_tx_enable(context, tx, 1)
    lte_calibtools.generic_valid_attn(context, set_attn_fn=set_attn_fn, power_fn=read_power_fn, criteria_prefix='FE_TSC%d32_' % lte_calibtools.real_tx(tx))

def validate_fb_attn(context, tx):

    #Requires working rfpal
    lte_calibtools.assert_rfpal_responsive(context)

    def set_attn_fn(value):
        lte_calibtools.set_fb_atten_raw(context, context.server.ssh, tx, value)
        time.sleep(1)

    def read_power_fn():
        pwr1 = lte_calibtools.read_rfpal_fb_dbm(context, context.server.ssh, tx)
        pwr2 = lte_calibtools.read_rfpal_fb_dbm(context, context.server.ssh, tx)
        return (pwr1 + pwr2)/2

    component_tester = context.server.component_tester
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    rfmeasure.set_generator_path(context, 'UUT_TX', tx)
    component_tester.setup_lte_bw10_uplink()
    component_tester.setup_measure(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_TX_INPUT_POWER_dBm)
    lte_calibtools.set_tx_enable(context, tx, 1)
    lte_calibtools.generic_valid_attn(context, set_attn_fn=set_attn_fn, power_fn=read_power_fn, criteria_prefix='FE_TSC%d33_' % lte_calibtools.real_tx(tx))

class TRXIsolation(testrunner.TestSuite):

    @testrunner.testcase("Validate TRX1 Isolation")
    def FE_TSC136(self, context):
        validate_tx_isolation(context, 0)

    @testrunner.testcase("Validate TRX2 Isolation")
    def FE_TSC236(self, context):
        validate_tx_isolation(context, 1)

def validate_tx_isolation(context, tx):
    rfmeasure.set_generator_path(context, 'UUT_TX', tx)
    component_tester = context.server.component_tester

    lte_calibtools.set_rx1_switches(context)
    lte_calibtools.set_rx_enable(context, tx, 1)
    lte_calibtools.set_rx_atten(context, context.server.ssh, tx, context.CALIBRATION_RX_ATTN_INIT_ATTEN)


    with context.criteria.evaluate_block() as eval_block:
        all_isolations = []
        for freq, name in zip(context.RF_DL_BMT, ['BOTTOM', 'MIDDLE', 'TOP']):
            rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
            component_tester.setup_tone()
            component_tester.setup_tone_measure(center_freq_mhz=freq, generator_power_dbm= context.RF_FE_TX_INPUT_POWER_dBm)

            power_at_output = component_tester.spectrum.find_peak_stable_amp(stable_diff=0.5)[1]
            rfmeasure.set_spectrum_path(context, 'UUT_RX', tx)

            power_at_rx = component_tester.spectrum.find_peak_stable_amp(stable_diff=0.5)[1]
            context.logger.info('Power at output = %.2f. Power at RX = %.2f' % (power_at_output, power_at_rx))
            trx_isolation = power_at_output-power_at_rx
            all_isolations = all_isolations + [trx_isolation]
            eval_block.evaluate('FE_TSC%d36_%s_TX_TO_RX_ISOLATION' % (lte_calibtools.real_tx(tx), name), trx_isolation)
        eval_block.evaluate('FE_TSC%d36_AVERAGE_TX_TO_RX_ISOLATION' % (lte_calibtools.real_tx(tx)), sum(all_isolations)/float(len(all_isolations)))

class TXRMSDetectorValidation(testrunner.TestSuite):

    fwd_rms_criteria = testrunner.CriteriaDeclaration(
        FWD_RMS= 'value > 200',
        REV_RMS= 'value > 80'
    )

    @testrunner.declare_criteria(fwd_rms_criteria)
    @testrunner.testcase("Validate TX1 RMS Detector FWD")
    def FE_TSC137(self, context):
        measure_rms_fwd_gain(context, 0)

    @testrunner.declare_criteria(fwd_rms_criteria)
    @testrunner.testcase("Validate TX2 RMS Detector FWD")
    def FE_TSC237(self, context):
        measure_rms_fwd_gain(context, 1)

def measure_rms_fwd_gain(context, tx):

    component_tester = context.server.component_tester
    rfmeasure.set_generator_path(context, 'UUT_TX', tx)
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)

    component_tester.setup_tone()
    component_tester.setup_tone_measure(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_TX_INPUT_POWER_dBm)
    lte_calibtools.set_tx_enable(context, tx, 1)
    lte_calibtools.set_tx_atten(context, context.server.ssh, tx, 6.0)

    time.sleep(1)

    rev_value = float(context.server.ssh_rms.read_rms('trx%d_rev' % tx))
    fwd_value = float(context.server.ssh_rms.read_rms('trx%d_fwd' % tx))
    with context.criteria.evaluate_block() as block:
        block.FWD_RMS(fwd_value)
        block.REV_RMS(rev_value)
    lte_calibtools.set_tx_enable(context, tx, 0)


##endregion
