import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure
import time

class TransceiverTXValidation(testrunner.TestSuite):

    @testrunner.suite_critical
    @testrunner.testcase("Validate comm. w/ RFPAL")
    def FE_PRP031(self, context):
        lte_calibtools.assert_rfpal_responsive(context)
        lte_calibtools.enable_fe_all_on(context)

    @testrunner.testcase("Validate TX1 RFPAL Range")
    def FE_TSC130(self, context):
        validate_tx_etm(context, 0)

    @testrunner.testcase("Validate TX2 RFPAL Range")
    def FE_TSC230(self, context):
        validate_tx_etm(context, 1)

    @testrunner.testcase("Validate 28V under load")
    def BB_TSC039(self, context):
        lte_calibtools.validate_both_pa_current(context)

def validate_tx_etm(context, tx):
    rfmeasure.set_spectrum_path(context, 'UUT_ANT', tx)
    spectrum = context.server.spectrum
    ssh = context.server.ssh

    context.logger.debug('RFPAL IN when off = %.2f' % lte_calibtools.read_rfpal_in_dbm(context, context.server.ssh, tx))
    lte_calibtools.reset_rfpal(context)
    lte_calibtools.set_tx_enable(context, tx, 1)
    bw = int(context.server.env_var.get_value('bw'))

    spectrum.setup_lte_dl_mode(lte_bw=bw, cont = 'ON')
    time.sleep(1)
    rfpal_in = context.logger.debug('RFPAL IN = %.2f' % lte_calibtools.read_rfpal_in_dbm(context, context.server.ssh, tx))
    context.criteria.evaluate('TSC%d30_RFPAL_INPUT_RANGE_dBm' % lte_calibtools.real_tx(tx), lte_calibtools.read_rfpal_in_dbm(context, context.server.ssh, tx))
