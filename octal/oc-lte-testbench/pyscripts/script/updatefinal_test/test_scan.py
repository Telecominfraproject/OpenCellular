import opentest
import opentest.script.testrunner as testrunner
from opentest.uut.uut import UUT, Format
import sys
from tools.lte import lte_calibtools, lte_scan

class BoardTestScan(testrunner.TestSuite):

    @testrunner.testcase("Scan BB", critical=True)
    def TP_PRP002(self, context):
        lte_scan.validate_bb_scan(context, context.ORIGINAL_UNIT_SCAN)
