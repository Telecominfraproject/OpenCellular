import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools

class Reset(testrunner.TestSuite):

    @testrunner.testcase("SW1 reset")
    def BB_TSC015(self, context):
        result = context.server.com.detect(target="board reset/factory default reset function...", timeout=120, keeplines=1000)
        assert result, "Boot failed, timeout reached"
        context.wait_tester_feedback("Press SW1 and hold it for at least 1 sec")

        result = context.server.com.detect(target="Reseting...|Rebooting...", timeout=120, keeplines=10000)#target = "login :" for full test
        assert result, "Reset fail, timeout reached"

        result = context.server.com.detect(target="Hit any key to stop autoboot", timeout=120, keeplines=10000)
        assert result, "Boot failed, timeout reached"

        context.logger.info("Reset occured")

    #@testrunner.testcase("Debug Reset Switch LED")
    def BB_TSC016(self, context):
        context.wait_tester_feedback("Hold SW2 and validate that LED8 is on when held (Only red LED on bottom)")

    @testrunner.testcase("SW2 reset")
    def BB_TSC017(self, context):
        context.wait_tester_feedback("Press SW2 and hold it for at least 1 sec")
        result = context.server.com.detect(target="Hit any key to stop autoboot", timeout=120, keeplines=10000)
