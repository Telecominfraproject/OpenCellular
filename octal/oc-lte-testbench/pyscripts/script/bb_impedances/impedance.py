import sys
import os
import opentest.script.testrunner as testrunner
from opentest.script import bringup_scripts

IMPEDANCE_TEXT="""With an Ohmmeter, verify that the impedances read at the requested test points are within the specified range."""
VOLTAGE_TEXT="""With an Volt meter, verify that the voltage read at the requested test points are within the specified range."""

BB_TOP_POINTS = [(1, '12VIN', 'J24', r"BB_BOT_1.jpg"),
                    (2, '5V', 'C211', r"BB_BOT_2.jpg"),
                    (3, '3V3', 'C257', r"BB_BOT_3.jpg"),
                    (4, 'EMMC_3V3', 'R237', r"BB_BOT_4.jpg"),
                    (5, 'FE_28V', 'C464', r"BB_BOT_5.jpg"),]

BB_BOTTOM_POINTS = [(1, '3V3_TIVA', 'C456',r"BB_TOP_1.jpg"),
                    (2, '2V5', 'C237',r"BB_TOP_2.jpg"),
                    (3, '1V8', 'C241',r"BB_TOP_3.jpg"),
                    (4, '1V5', 'C247',r"BB_TOP_4.jpg"),
                    (5, '1V35', 'C279',r"BB_TOP_5.jpg"),
                    (6, '7130_VDD', 'C215',r"BB_TOP_6.jpg"),
                    (7, 'USB_5V', 'C223',r"BB_TOP_7.jpg"),
                    (8, 'FE_5V', 'C486',r"BB_TOP_8.jpg"),
                    (9, 'FE_3V3', 'C515',r"BB_TOP_9.jpg"),
                    (10, 'FE_1V8', 'C475',r"BB_TOP_10.jpg")]


class BBImpedance(testrunner.TestSuite):

    @testrunner.testcase("BB Impedance Test", critical=True)
    def BB_TSC001(self, context):

        context.wait_tester_feedback("Ground here",image=os.path.join(context.IMAGE_FOLDER, r"BB_TOP_PROBE.jpg"))
        bringup_scripts.impedance_test(context, BB_TOP_POINTS, 'BB_TSC001', base_path=context.IMAGE_FOLDER)
        context.wait_tester_feedback("Ground here",image=os.path.join(context.IMAGE_FOLDER, r"BB_BOT_PROBE.jpg"))
        bringup_scripts.impedance_test(context, BB_BOTTOM_POINTS, 'BB_TSC001', base_path=context.IMAGE_FOLDER)

    @testrunner.testcase("BB Voltages Test", critical=True)
    def BB_TSC002(self, context):
        context.wait_tester_feedback("Power up the BB board using a 12V source")
        bringup_scripts.voltage_test(context, BB_TOP_POINTS, 'BB_TSC002', base_path=context.IMAGE_FOLDER)
        bringup_scripts.voltage_test(context, BB_BOTTOM_POINTS, 'BB_TSC002', base_path=context.IMAGE_FOLDER)
