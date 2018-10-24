import sys
import os
import opentest.script.testrunner as testrunner
from opentest.script import bringup_scripts


IMPEDANCE_TEXT="""With an Ohmmeter, verify that the impedances read at the requested test points are within the specified range."""
VOLTAGE_TEXT="""With an Volt meter, verify that the voltage read at the requested test points are within the specified range."""

FE_POINTS =[(1, '28V', 'TP34',r"FE_revE_1.jpg"),
            (2, '5V', 'TP38',r"FE_revE_2.jpg"),
            (3, '3V3', 'TP82',r"FE_revE_3.jpg"),
            (4, '1V8', 'TP37',r"FE_revE_4.jpg"),
            (5, '3V3_TIVA', 'TP39',r"FE_revE_5.jpg"),
            (6, '28V_PA1', 'TP99',r"FE_revE_6.jpg"),
            (7, '28V_PA2', 'TP35',r"FE_revE_7.jpg"),
            (8, '28V_PA0_SW', 'C38',r"FE_revE_8.jpg"),
            (9, '28V_PA1_SW', 'C35',r"FE_revE_9.jpg")]


class FEImpedance(testrunner.TestSuite):

    @testrunner.testcase("FE Impedance Test", critical=True)
    def FE_TSC001(self, context):
        bringup_scripts.impedance_test(context, FE_POINTS, 'FE_TSC001', base_path=context.IMAGE_FOLDER)
