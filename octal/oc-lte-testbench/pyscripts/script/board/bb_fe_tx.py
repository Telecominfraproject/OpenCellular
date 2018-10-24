import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure

class RMS1DetectorOutputValidation(testrunner.TestSuite):

    @testrunner.testcase("Validate Output Power TX1")
    def FE_TSC134(self, context):
        validate_output_power(context, 0)

    @testrunner.testcase("Validate TX1 RMS Detector Output On")
    def FE_TSC135(self, context):
        validate_tx_rms(context, 0)

class RMS2DetectorOutputValidation(testrunner.TestSuite):

    @testrunner.testcase("Validate Output Power TX2")
    def FE_TSC234(self, context):
        validate_output_power(context, 1)

    @testrunner.testcase("Validate TX2 RMS Detector Output On")
    def FE_TSC235(self, context):
        validate_tx_rms(context, 1)

def validate_output_power(context, tx):
    lte_calib.calibrate_single_freq(context, tx, context.RF_DL_MIDDLE_FREQ) # Get 30 dBm output
    with context.criteria.evaluate_block() as eval_block:

        aclr = context.server.spectrum.find_stable_aclr()
        pwr = lte_calib.read_output_power(context, context.server.spectrum)

        context.server.spectrum.read_evm()
        evm = context.server.spectrum.fetch_evm()

        eval_block.evaluate('FE_TSC%d34_OUTPUT_POWER' % lte_calibtools.real_tx(tx), pwr)
        eval_block.evaluate('FE_TSC%d34_PA_CURRENT' % lte_calibtools.real_tx(tx), context.server.sensor.current_tx(tx))
        eval_block.evaluate('FE_TSC%d34_EVM' % lte_calibtools.real_tx(tx), evm)
        eval_block.evaluate('FE_TSC%d34_ACLR_dBc' % lte_calibtools.real_tx(tx), aclr)

def validate_tx_rms(context, tx):

    lte_calib.acquire_acp(context)
    output_pwr = lte_calib.read_output_power(context, context.server.spectrum)

    fwd_diff, rev_diff = lte_calibtools.get_rms_errors_from_prediction(context, tx, output_pwr)
    fwd = context.server.ssh_rms.read_rms('trx%d_fwd' % tx)
    rev = context.server.ssh_rms.read_rms('trx%d_rev' % tx)

    with context.criteria.evaluate_block() as eval_block:
        eval_block.evaluate('FE_TSC%d35_FWD_VALUE_30_dBm' % lte_calibtools.real_tx(tx), fwd)
        eval_block.evaluate('FE_TSC%d35_REV_VALUE_30_dBm' % lte_calibtools.real_tx(tx), rev)
        eval_block.evaluate('FE_TSC%d35_FWD_ERROR_30_dBm' % lte_calibtools.real_tx(tx), fwd_diff)
        eval_block.evaluate('FE_TSC%d35_REV_ERROR_30_dBm' % lte_calibtools.real_tx(tx), rev_diff)
