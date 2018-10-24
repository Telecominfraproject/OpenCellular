import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure
import time

class TransceiverRXValidation(testrunner.TestSuite):

    @testrunner.testcase("Set TX off")
    def BB_PRP050(self, context):
        lte_calibtools.set_all_tx_off(context)

    @testrunner.testcase("Transceiver RX1")
    def BB_TSC150(self, context):
        validate_rssi(context, 0)

    @testrunner.testcase("Transceiver RX2")
    def BB_TSC250(self, context):
        validate_rssi(context, 1)

def validate_rssi(context, tx):
    rfmeasure.set_generator_path(context, 'UUT_ANT', tx)
    lte_calibtools.set_rx_enable(context, tx, 1)
    lte_calibtools.set_rx_atten(context, context.server.ssh, tx, context.CALIBRATION_RX_ATTN_INIT_ATTEN)
    if not hasattr(context, 'CALIBRATION_RX%d_GAIN' % tx):
        lna_gain = 12
    else:
        lna_gain = getattr(context, 'CALIBRATION_RX%d_GAIN' % tx)

    lte_calib.validate_bb_rssi(context, tx, path_gain=lna_gain, with_feedback=False)
