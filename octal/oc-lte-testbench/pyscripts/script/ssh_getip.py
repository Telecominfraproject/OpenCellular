import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools

class SSGGetIP(testrunner.TestSuite):

    @testrunner.testcase("Get-IP", critical=True)
    def BB_TSC004(self, context):
        ssh_getip(context)

    @testrunner.testcase("Launch SSH Service", critical=True)
    def CM_PRP017(self, context):
        lte_calibtools.launchssh(context)

    @testrunner.testcase("Launch SCP Service", critical=True)
    def CM_PRP018(self, context):
        lte_calibtools.launchscp(context)


def ssh_getip(context):
        lte_calibtools.login_from_com(context)

        context.logger.info("Login sucessful")
        context.UUT_IP_ADDR = context.server.com.get_ip(context.UUT_IP_ETH).strip()
        assert len(context.UUT_IP_ADDR) < 16 and len(context.UUT_IP_ADDR) > 5, "IP addr does not appear to be present"
        context.logger.info("Addr IP :" + context.UUT_IP_ADDR)

        context.UUT_MAC_ADDR = context.server.com.get_mac(context.UUT_IP_ETH).strip()
        assert len(context.UUT_MAC_ADDR) < 18 and len(context.UUT_MAC_ADDR) > 12, "MAC addr does not appear to be present"
        context.logger.info("Addr MAC :" + context.UUT_MAC_ADDR)
