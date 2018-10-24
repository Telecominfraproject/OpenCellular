from nute.core import TestSuite, testcase, declare_criteria, CriteriaDeclaration
from opentest.integration import rfmeasure
from tools.lte import lte_calibtools

class SnifferTest(TestSuite):

    @declare_criteria(CriteriaDeclaration(
        POWER_LOSS='value < 5'
    ))
    @testcase('Sniffer validation')
    def FE_TSC250(self, context):
        context.wait_tester_feedback('Please plug the cable named RX2 into the connector BB Sniffer (next to ANT2)')

        component_tester = context.server.component_tester

        lte_calibtools.set_sniffer_active(context, True)

        rfmeasure.set_generator_path(context, 'UUT_ANT', 1)
        rfmeasure.set_spectrum_path(context, 'UUT_RX', 1)

        component_tester.setup_tone()
        component_tester.setup_tone_measure(center_freq_mhz=context.RF_DL_MIDDLE_FREQ, generator_power_dbm= context.RF_FE_TX_INPUT_POWER_dBm)

        power_at_sniffer = component_tester.spectrum.find_peak_stable_amp(stable_diff=0.5)[1]

        lte_calibtools.set_sniffer_active(context, False)
        context.criteria.POWER_LOSS(context.RF_FE_TX_INPUT_POWER_dBm - power_at_sniffer)
