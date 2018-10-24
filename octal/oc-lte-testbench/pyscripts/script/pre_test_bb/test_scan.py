import opentest.script.testrunner as testrunner
from tools.lte import lte_scan

class BoardTestScan(testrunner.TestSuite):
    @testrunner.testcase("Validate BB Scan", critical=True)
    def CM_PRP001(self, context):
        lte_scan.validate_bb_scan(context, context.ORIGINAL_UNIT_SCAN)
