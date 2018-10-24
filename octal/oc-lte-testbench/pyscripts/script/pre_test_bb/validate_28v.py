import sys
import os
import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools
import time

class Validate28V(testrunner.TestSuite):

    @testrunner.testcase("Validate PA 1 Enable Current")
    def BB_TSC118(self, context):
        test_pa_enable(context, 0)

    @testrunner.testcase("Validate PA 1 Enable Current")
    def BB_TSC218(self, context):
        test_pa_enable(context, 1)

def test_pa_enable(context, tx):
    lte_calibtools.set_pa(context, tx, 0)
    time.sleep(0.5)
    current_off = context.server.sensor.current_tx(tx)
    lte_calibtools.set_pa(context, tx, 1)
    time.sleep(0.5)
    current_on = context.server.sensor.current_tx(tx)
    voltage_on = context.server.sensor.voltage_tx(tx)
    lte_calibtools.set_pa(context, tx, 0)

    with context.criteria.evaluate_block() as eval_block:
        eval_block.evaluate('BB_TSC%s18_PA_CURRENT_OFF' % lte_calibtools.real_tx(tx), current_off)
        eval_block.evaluate('BB_TSC%s18_PA_CURRENT_ON' % lte_calibtools.real_tx(tx), current_on)
        eval_block.evaluate('BB_TSC%s18_PA_VOLTAGE_ON' % lte_calibtools.real_tx(tx), voltage_on)
