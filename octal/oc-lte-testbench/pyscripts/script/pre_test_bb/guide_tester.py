import sys
import os
import opentest.script.testrunner as testrunner
from opentest.integration import rfmeasure

class PreTestBBTestbench(testrunner.TestSuite):

    @testrunner.testcase("Setup unit", critical=True)
    def BB_PRP000(self, context):
        context.wait_tester_feedback("""
Plug in the COM port USB cable
Plug in the Ethernet cable
Plug in the GPS antenna
Plug in the four TRX cables based on their identifications
Place an SD card in the MMC card slot

Slide in a golden Front-End board

Plug in the 12V power header
        """)
