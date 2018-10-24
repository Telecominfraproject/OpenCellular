from nute.core import *
import sys
from tools.lte import lte_calibtools, lte_scan

class BoardTestScan(TestSuite):
    @testcase("Validate BB Scan", critical=True)
    def CM_PRP001(self, context):
        lte_scan.validate_bb_scan(context, context.ORIGINAL_UNIT_SCAN)

    @testcase("Scan FE", critical=True)
    def CM_PRP002(self, context):
        scan = context.wait_tester_scan_unit("Please scan the FE Unit")
        lte_scan.validate_fe_scan(context, scan)
