import opentest.script.testrunner as testrunner
from tools.lte import lte_calibtools
import time
import os

INSTRUCTION_SETUP = """Follow red frame steps
1. Disable airplane mode
In setting :
3. Go to "mobile networks"
4. Go to "Network operators"
5. Click "Select automatically"
If the connection succed, it should show as in frame in blue : Amarisolft Network.
"""
INSTRUCTION_HELP = """Follow red frame steps to setup your connection manually
1. Disable airplane mode
In setting :
3. Go to "mobile networks"
4. Go to "Network operators"
5. Click "Search networks" and chose "Amarisolft Network" when available*
*Note : It make take a few minutes to search for available networks
"""
INFO="""

Local  Host is %s connected to port %s
Remote Host is %s connected to port %s

Run length (seconds)       : %s
Number of bytes transfered : %s

%s bits per second

Additional Info

Host   Utilization: %s
Remote Utilization: %s
Interval Length   : %s

"""
class Transmission(testrunner.TestSuite):

    #@testrunner.testcase("Transmission Test", critical=False)
    def lCTESTCl_A111(self, context):
        try :
            context.wait_tester_feedback(INSTRUCTION_SETUP, button=testrunner.PopupType.PASS_FAIL, image=r"support\image\automatically_e2e.png")
        except testrunner.TesterInterruption:
            self.popup_help(context)

        #context.wait_tester_feedback("#TODO")

        #self.evaluate(context, "TRANSMISSION_RUN_LENGHT", value_run)
        #self.evaluate(context, "TRANSMISSION_BITS_PER_SECOND", value_bps)


    @testrunner.testcase("Power up", critical=True)
    def SY_PRP064(self, context):

        context.wait_tester_feedback("Wait until all leds are Green, Then plug the Mobile Broadband",image=os.path.join(context.IMAGE_FOLDER, r"device_ok.jpg"))

    #@testrunner.testcase("Set RX attenuators", critical=True)
    def SY_PRP065(self, context):
        lte_calibtools.set_rx_atten(context, context.server.ssh, 0, 31.75)
        lte_calibtools.set_rx_atten(context, context.server.ssh, 1, 0)

    @testrunner.testcase("UE/ENB checkup", critical=True)
    def SY_TSC060(self, context):

        while True:
            try :
                if not context.server.e2e_ssh.enb_connected():
                    context.wait_tester_feedback("NO ENB CONNECTED\n Check your OC-LTE\n Might take a few minutes",fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
                break
            except testrunner.TesterRetry:
                pass
        while True:
            try :
                if not context.server.e2e_ssh.ue_connected():
                    context.wait_tester_feedback("Open Mobile Partner\nCheck if the required field match\nThen Connect",image=os.path.join(context.IMAGE_FOLDER, r"mobile_partner_1.png"),fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
                    context.wait_tester_feedback("NO UE CONNECTED\n Check your OC-LTE and your Mobile Broadband\nFirst time click retry",fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
                #context.logger.debug(context.server.e2e_ssh.ue_info())
                break
            except testrunner.TesterRetry:
                pass
        #assert context.server.e2e_ssh.ue_connected(),"No UE Detected"

    @testrunner.testcase("Validate Internet Connection", critical=False)
    def SY_TSC061(self, context):
        while True:
            try:
                context.wait_tester_feedback("Check if the required field match\n Else Retry ",image=os.path.join(context.IMAGE_FOLDER, r"mobile_partner_2.png"), fb_type= testrunner.FeedbackType.PASS_FAIL_RETRY)
                break
            except testrunner.TesterRetry:
                pass

    @testrunner.testcase("Transmission Test", critical=False)
    def SY_TSC062(self, context):
        context.logger.info(("test time : "+ context.E2E_TRANSMISSION + " sec"))
        self.log_e2e_n = os.path.join(context.LOGS_PATH, "e2e_normal.txt")

        context.server.e2e_ssh.iperf_launch_server_remote(logfile="log.txt")
        context.server.e2e_ssh.iperf_run_client_local(logfile=self.log_e2e_n, arguments=['-t', context.E2E_TRANSMISSION])

        context.server.e2e_ssh.iperf_killremote()
        time.sleep(1)

        try:
            result_n = context.server.e2e_ssh.extract(self.log_e2e_n)
            value_upload = (float(result_n.sent_bps)/(1024*1024))
            context.logger.info("Upload Speed: "+str(value_upload)+ " Mbit/sec")

            context.logger.debug(INFO%(result_n.local_host,result_n.local_port,result_n.remote_host,result_n.remote_port,result_n.runLength,result_n.bytes,result_n.sent_bps,
                                result_n.hostUtilization,result_n.remoteUtilization,result_n.intervalLength))
            context.criteria.evaluate('TRANSMISSION_MBS',value_upload)
        except KeyError:
            context.wait_tester_feedback("Transmission Fail")
            raise

    @testrunner.testcase("Reception Test", critical=False)
    def SY_TSC063(self, context):
        context.logger.info(("test time : " + context.E2E_RECEPTION + " sec"))
        self.log_e2e_r = os.path.join(context.LOGS_PATH, "e2e_reverse.txt")

        context.server.e2e_ssh.iperf_launch_server_remote(logfile="log.txt")
        context.server.e2e_ssh.iperf_run_client_local_reverse(logfile=self.log_e2e_r, arguments=['-t', context.E2E_RECEPTION])
        context.server.e2e_ssh.iperf_killremote()

        try:
            result_r = context.server.e2e_ssh.extract(self.log_e2e_r)
            value_download = (float(result_r.received_bps)/(1024*1024))
            context.logger.info("Download Speed: "+str(value_download)+ " Mbit/sec")

            context.logger.debug(INFO%(result_r.local_host,result_r.local_port,result_r.remote_host,result_r.remote_port,result_r.runLength,result_r.bytes,result_r.received_bps,
                                result_r.hostUtilization,result_r.remoteUtilization,result_r.intervalLength))
            context.criteria.evaluate('RECEPTION_MBS', value_download)
        except KeyError:
            context.wait_tester_feedback("Reception Fail")
            raise

    def popup_help(self, context):
        context.wait_tester_feedback(INSTRUCTION_HELP, button=testrunner.PopupType.PASS_FAIL, image=r"support\image\manual_e2e.png")
