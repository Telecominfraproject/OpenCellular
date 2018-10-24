import sys
import os
import opentest.script.testrunner as testrunner
from opentest.integration import rfmeasure

class SystemTestbench(testrunner.TestSuite):

    @testrunner.testcase("Setup unit", critical=True)
    def SY_PRP000(self, context):
        context.wait_tester_feedback("""
Plug in the Ethernet cable
Plug in the GPS antenna
Plug in the two TRX cables based on their identifications
Plug in the power cord
        """, image=os.path.join(context.IMAGE_FOLDER, r"SYSTEM_E2E.jpg"))
