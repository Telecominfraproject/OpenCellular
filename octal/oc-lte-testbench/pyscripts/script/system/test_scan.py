import opentest
import opentest.script.testrunner as testrunner
from opentest.uut.uut import UUT, Format
import sys
from tools.lte import lte_calibtools, lte_scan

class BoardTestScan(testrunner.TestSuite):


    @testrunner.testcase("Validate System Scan", critical=True)
    def TP_PRP001(self, context):
        lte_scan.validate_sys_scan(context, context.ORIGINAL_UNIT_SCAN)

    @testrunner.testcase("Scan BB", critical=True)
    def TP_PRP002(self, context):
        scan = context.wait_tester_scan_unit("Please scan the BB Unit")
        lte_scan.validate_bb_scan(context, scan)

    @testrunner.testcase("Scan FE", critical=True)
    def TP_PRP003(self, context):
        scan = context.wait_tester_scan_unit("Please scan the FE Unit")
        lte_scan.validate_fe_scan(context, scan)
