import sys
import os
import opentest.script.testrunner as testrunner
from opentest.script import bringup_scripts

IMPEDANCE_TEXT="""With an Ohmmeter, verify that the impedances read at the requested test points are within the specified range."""
VOLTAGE_TEXT="""With an Volt meter, verify that the voltage read at the requested test points are within the specified range."""

FE_POINTS =[(1, '28V', 'TP24',r"FE_1.jpg"),
            (2, '5V', 'TP25',r"FE_2.jpg"),
            (3, '3V3', 'TP34',r"FE_3.jpg"),
            (4, '1V8', 'TP85',r"FE_4.jpg"),
            (5, '3V3_TIVA', 'TP41',r"FE_5.jpg"),
            (6, '28V_PA1', 'TP102',r"FE_6.jpg"),
            (7, '28V_PA2', 'TP23',r"FE_7.jpg"),
            (8, '28V_PA0_SW', 'C17',r"FE_8.jpg"),
            (9, '28V_PA1_SW', 'C15',r"FE_9.jpg")]

FE_POT_POINTS =[(1, 'VGS_TX1_1A', 'C133',r"FE_POT_TX1_1A.jpg"),
                (2, 'VGS_TX1_2A', 'C134',r"FE_POT_TX1_2A.jpg"),
                (3, 'VGS_TX1_1B', 'C129',r"FE_POT_TX1_1B.jpg"),
                (4, 'VGS_TX1_2B', 'C128',r"FE_POT_TX1_2B.jpg"),
                (5, 'VGS_TX2_1A', 'C163',r"FE_POT_TX2_1A.jpg"),
                (6, 'VGS_TX2_2A', 'C164',r"FE_POT_TX2_2A.jpg"),
                (7, 'VGS_TX2_1B', 'C159',r"FE_POT_TX2_1B.jpg"),
                (8, 'VGS_TX2_2B', 'C158',r"FE_POT_TX2_2B.jpg")]

class FEImpedance(testrunner.TestSuite):

    @testrunner.testcase("FE Impedance Test", critical=True)
    def FE_TSC001(self, context):
        context.wait_tester_feedback("Ground here",image=os.path.join(context.IMAGE_FOLDER, r"FE_PROBE.jpg"))
        bringup_scripts.impedance_test(context, FE_POINTS, 'FE_TSC001', base_path=context.IMAGE_FOLDER)

    @testrunner.testcase("Power-up", critical=True)
    def CM_PRP030(self, context):
        context.wait_tester_feedback("Connect the Front-End to its paired Baseband board")
        try:
            context.logger.info("Power On")
            context.server.dcsupply.setvoltage(context.BOARD_VOLTAGE)
            context.server.dcsupply.setenable(1)
        except:
            context.wait_tester_feedback("Unable to automatically power on the board; please power manually")

    @testrunner.testcase("Potentiometer Tuning Test", critical=True)
    def FE_TSC002(self, context):
        context.wait_tester_feedback("Tune potentiometer R301 until voltage on C133 is exactly 1.88V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX1_1A.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)
        context.wait_tester_feedback("Tune potentiometer R302 until voltage on C134 is exactly 1.88V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX1_2A.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)
        context.wait_tester_feedback("Tune potentiometer R275 until voltage on C129 is exactly 1.43V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX1_1B.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)
        context.wait_tester_feedback("Tune potentiometer R274 until voltage on C128 is exactly 1.36V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX1_2B.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)
        context.wait_tester_feedback("Tune potentiometer R356 until voltage on C163 is exactly 1.88V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX2_1A.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)
        context.wait_tester_feedback("Tune potentiometer R357 until voltage on C164 is exactly 1.88V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX2_2A.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)
        context.wait_tester_feedback("Tune potentiometer R330 until voltage on C159 is exactly 1.43V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX2_1B.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)
        context.wait_tester_feedback("Tune potentiometer R329 until voltage on C158 is exactly 1.36V  ",image=os.path.join(context.IMAGE_FOLDER, r"FE_POT_TX2_2B.jpg"),fb_type=testrunner.FeedbackType.PASS_FAIL)

        context.wait_tester_feedback("Use the laptop and the SC2200 GUI to set RFPAL frequencies of both channel between 1800 and 1900 MHz.")

    @testrunner.testcase("Power-off", critical=True)
    def CM_CLN030(self, context):
        try:
            context.logger.info("Power Off")
            context.server.dcsupply.setenable(0)
        except:
            context.wait_tester_feedback("Power OFF the Front-End to allow impedance measurements")

    @testrunner.testcase("Potentiometer Impedance Measure", critical=True)
    def FE_TSC003(self, context):

        bringup_scripts.impedance_test(context, FE_POT_POINTS, 'FE_TSC003', base_path=context.IMAGE_FOLDER)
