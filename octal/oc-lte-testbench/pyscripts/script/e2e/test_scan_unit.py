import opentest
import opentest.script.testrunner as testrunner
import sys
from tools.lte import lte_calibtools, lte_scan

class BoardTestScan(testrunner.TestSuite):
    @testrunner.testcase("Validate SYS Scan", critical=True)
    def CM_PRP001(self, context):
        lte_scan.validate_unit_scan(context, context.ORIGINAL_UNIT_SCAN)
