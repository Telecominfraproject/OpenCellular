import sys
import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools
from tools.lte import lte_calib
from opentest.integration import rfmeasure
import time

class UFLValidation(testrunner.TestSuite):

    @testrunner.testcase("Validate TX1 cables")
    def FE_TSC132(self, context):
        validate_tx_cables(context, 0)

    @testrunner.testcase("Validate TX2 cables")
    def FE_TSC133(self, context):
        validate_tx_cables(context, 1)

    @testrunner.testcase("Validate RX1 cables")
    def FE_TSC232(self, context):
        while True:
            try :
                validate_rx_cables(context, 0)
                context.wait_tester_feedback("RX1: Replay sequence ?",fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
                break
            except testrunner.TesterRetry:
                pass

    @testrunner.testcase("Validate RX2 cables")
    def FE_TSC233(self, context):
        while True:
            try :
                validate_rx_cables(context, 1)
                context.wait_tester_feedback("RX2: Replay sequence ?",fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
                break
            except testrunner.TesterRetry:
                pass


def validate_tx_cables(context, tx):

    component_tester = context.server.component_tester

    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    lte_calibtools.default_atten_values(context, tx)
    bw = int(context.server.env_var.get_value('bw'))

    context.server.spectrum.setup_lte_dl_mode(lte_bw=bw, cont = 'ON')
    lte_calib.setup_etm_measurement(context, context.RF_DL_MIDDLE_FREQ)

    lte_calibtools.set_tx_enable(context, tx, 1)

    context.wait_tester_feedback('Checking path TX %d' % lte_calibtools.real_tx(tx))

def validate_rx_cables(context, tx):
    rfmeasure.set_generator_path(context, 'UUT_ANT', tx)
    lte_calibtools.set_rx_enable(context, tx, 1)
    lte_calibtools.set_rx_atten(context, context.server.ssh, tx, context.CALIBRATION_RX_ATTN_INIT_ATTEN)
    if not hasattr(context, 'CALIBRATION_RX%d_GAIN' % tx):
        lna_gain = 12
    else:
        lna_gain = getattr(context, 'CALIBRATION_RX%d_GAIN' % tx)
    path_gain = lna_gain
    context.server.generator.setup_waveform_in_gen("10MHZ_FRCA13_RO_0")
    context.server.generator.execute_command(':RAD:ARB:SCL:RATE 15.36MHz')
    lte_calibtools.set_rx_freq(context, context.RF_UL_MIDDLE_FREQ)

    with context.criteria.evaluate_block() as block:
        for dbm in [-30, -40, -50]:
            context.server.generator.setup_output(center_freq_mhz=context.RF_UL_MIDDLE_FREQ, power_dbm= dbm - path_gain)
            context.server.generator.output_enable(True, mod_enable=True)
            time.sleep(context.RF_STABILISATION_TIME*2)
            rssi = lte_calibtools.read_bb_rssi(context, tx)
            rssi_pow = lte_calib.estimate_power_from_rssi(context, rssi)
            context.logger.info("RSSI %d dBm signal= %d -> %.2f dBm" % (dbm, rssi, rssi_pow))

        context.server.generator.output_enable(False, mod_enable=False)
