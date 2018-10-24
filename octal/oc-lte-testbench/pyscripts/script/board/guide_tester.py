import sys
import os
import opentest.script.testrunner as testrunner
from opentest.integration import rfmeasure

class BoardTestbench(testrunner.TestSuite):

    @testrunner.testcase("Setup unit", critical=True)
    def CM_PRP000(self, context):
        context.wait_tester_feedback("""
[BB] Plug in the COM port USB cable
[BB] Plug in the Ethernet cable
[BB] Plug in the GPS antenna
[BB] Plug in the four TRX cables based on their identifications
[BB] Place an SD card in the MMC card slot

[BB] Plug in the four BB TRX u.fl cables on the BB Board
[FE] Plug in the four FE TRX u.fl cables on the FE Board
[FE] Screw in the two ANT SMA cables on the FE board
[FE] Plug in the LED BOARD

[BB+FE] Slide the two boards together

[BB] Plug in the 12V power header
        """)
        context.wait_tester_feedback("""
Make sure the four TX/RX cables from the switch are connected in their respective FE TX/RX
        """)
        context.wait_tester_feedback("""
Make sure the four jumpers are present on the BB Board
        """)
