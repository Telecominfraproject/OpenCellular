import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools

class UpdateSetup(testrunner.TestSuite):

    @testrunner.testcase("Update Test Setup", critical=True)
    def CM_PRP042(self, context):
        lte_calibtools.updatesetup_transfer_files(context)
