import opentest
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools, lte_calib
from opentest.integration import rfmeasure
import time

class TransceiverBBOnlyRXValidation(testrunner.TestSuite):

    @testrunner.testcase("Transceiver RX1 RSSI")
    def BB_TSC151(self, context):
        validate_rx_bb_only_etm(context, 0)

    #@testrunner.testcase("Transceiver RX1 Sensitivity")
    def BB_TSC152(self, context):
        calculate_sensitivity(context, 0)

    @testrunner.testcase("Transceiver RX2 RSSI")
    def BB_TSC251(self, context):
        validate_rx_bb_only_etm(context, 1)

    #@testrunner.testcase("Transceiver RX2 Sensitivity")
    def BB_TSC252(self, context):
        calculate_sensitivity(context, 1)

def validate_rx_bb_only_etm(context, tx):
    rfmeasure.set_generator_path(context, 'BB_RX', tx)
    lte_calib.validate_bb_rssi(context, tx, with_feedback=False)


def calculate_sensitivity(context, tx):
    rfmeasure.set_generator_path(context, 'BB_RX', tx)
    lte_calib.validate_bb_sensitivity(context, tx, with_feedback=False)
