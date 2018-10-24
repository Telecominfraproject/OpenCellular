import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools

class AssembleUnit(testrunner.TestSuite):

    @testrunner.testcase("BB and FE RF Jumpers", critical=True)
    def CM_PRP060(self, context):
        lte_calibtools.default_atten_values(context, 0)
        lte_calibtools.default_atten_values(context, 1)
        lte_calibtools.set_all_tx_off(context)
        context.wait_tester_feedback('Please disconnect Front-End TXs, RXs from the RFSwitch and link the BB and FE TXs, RXs with jumpers.')
