import opentest.script.testrunner as testrunner


##region TSCx30: RFPAL Range Validation

class HoldTest(testrunner.TestSuite):

    @testrunner.testcase("Wait Enter", critical=True)
    def SY_PRP300(self, context):
        context.wait_tester_feedback("Press enter to continue")
