import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure
import time

class RMS1DetectorInputValidation(testrunner.TestSuite):

    @testrunner.not_implemented
    @testrunner.testcase("Validate TX1 RMS Detector Input")
    def FE_TSC143(self, context):
        validate_rx_rms(context, 0)

class RMS2DetectorInputValidation(testrunner.TestSuite):

    @testrunner.not_implemented
    @testrunner.testcase("Validate TX2 RMS Detector Input")
    def FE_TSC243(self, context):
        validate_rx_rms(context, 1)

def validate_rx_rms(context, tx):
    lte_calibtools.set_tx_enable(context, tx, 0)
    rfmeasure.set_generator_path(context, 'UUT_ANT', tx)
    context.server.component_tester.setup_lte_bw10_uplink()
    context.server.generator.setup_output(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, power_dbm=context.CALIBRATION_RX_DL_INPUT_MAX_POWER_dBm)
    context.server.generator.output_enable(True, True)

    time.sleep(3)

    fwd_diff, rev_diff = lte_calibtools.get_rms_errors_from_prediction(context, tx, context.CALIBRATION_RX_DL_INPUT_MAX_POWER_dBm)

    with context.criteria.evaluate_block() as eval_block:
        eval_block.evaluate('FE_TSC%d43_FWD_ERROR' % lte_calibtools.real_tx(tx), fwd_diff)
        eval_block.evaluate('FE_TSC%d43_REV_ERROR' % lte_calibtools.real_tx(tx), rev_diff)
