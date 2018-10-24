from opentest.script.testrunner import TestSuite,testcase
from tools.lte import lte_calibtools
import time
import os

class Cleanup(TestSuite):
    
    @testcase("Kill server")
    def CM_CLN001(self, context):
        import opentest.script.server_scripts as server_scripts
        #context.wait_tester_feedback("Shutdown server ?")
        server_scripts.shutdown(context)
