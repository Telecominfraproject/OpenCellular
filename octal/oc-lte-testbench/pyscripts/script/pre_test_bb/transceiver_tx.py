import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure
import time

class TransceiverBBOnlyTXValidation(testrunner.TestSuite):

    @testrunner.testcase("OCXO Calibration")
    def BB_TSC020(self, context):
        calibrate_ocxo(context)

    @testrunner.testcase("Transceiver TX1")
    def BB_TSC130(self, context):
        validate_tx_etm(context, 0)

    @testrunner.testcase("Transceiver TX2")
    def BB_TSC230(self, context):
        validate_tx_etm(context, 1)

def validate_tx_etm(context, tx):
    rfmeasure.set_spectrum_path(context, 'BB_TX', tx)

    lte_calibtools.set_bb_atten(context, context.server.ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)

    bw = int(context.server.env_var.get_value('bw'))

    context.server.spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')
    lte_calib.setup_etm_measurement(context, context.RF_DL_MIDDLE_FREQ)
    time.sleep(context.RF_STABILISATION_TIME)
    lte_calib.acquire_acp(context)

    normal_power = lte_calib.read_output_power(context, context.server.spectrum)

    lte_calibtools.set_bb_atten(context, context.server.ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN - context.TRANSCEIVER_OUTPUT_GAIN_TEST)
    lte_calibtools.cn_rfdriver_command_list(context, 'rag')
    time.sleep(context.RF_STABILISATION_TIME)
    lte_calib.acquire_acp(context)
    with_gain_power = lte_calib.read_output_power(context, context.server.spectrum)
    diff = with_gain_power-normal_power
    aclr = context.server.spectrum.find_stable_aclr()
    context.server.spectrum.read_evm()
    evm = context.server.spectrum.fetch_evm()

    with context.criteria.evaluate_block() as eval_block:
        context.criteria.evaluate('BB_TSC%d30_OUTPUT_POWER_dBm' % lte_calibtools.real_tx(tx), normal_power)
        context.criteria.evaluate('BB_TSC%d30_OUTPUT_POWER_GAIN_dB' % lte_calibtools.real_tx(tx), diff)
        context.criteria.evaluate('BB_TSC%d30_OUTPUT_ACLR' % lte_calibtools.real_tx(tx), aclr)
        context.criteria.evaluate('BB_TSC%d30_OUTPUT_EVM' % lte_calibtools.real_tx(tx), evm)



def calibrate_ocxo(context):
    tx=0
    rfmeasure.set_spectrum_path(context, 'BB_TX', tx)
    lte_calibtools.set_bb_atten(context, context.server.ssh, tx, context.CALIBRATION_BB_ATTN_INIT_ATTEN)

    bw = int(context.server.env_var.get_value('bw'))
    context.server.spectrum.setup_lte_dl_mode(lte_bw=bw, cont='ON')
    lte_calib.setup_etm_measurement(context, context.RF_DL_MIDDLE_FREQ)

    lte_calib.calibrate_ocxo_no_setup(context)
