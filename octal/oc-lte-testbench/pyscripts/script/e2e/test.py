import sys
import os
import opentest.script.testrunner as testrunner

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

class Preparation(testrunner.TestSuite):

    @testrunner.testcase("Launch Service Server", critical=True)
    def CM_PRP010(self, context):
        import opentest.script.server_scripts as server_scripts
        server_scripts.launch(context)
        #context.server.open_logs_request()

        server_path = os.path.join(context.LOGS_PATH, "server")
        context.server.set_log_destination(server_path)

    @testrunner.testcase("Initial setup", critical=True)
    def SY_PRP061(self, context):

        result_r = context.server.e2e_ssh.extract(r"C:\Users\production\Desktop\e2e_reverse.txt")
        print (INFO%(result_r.local_host,result_r.local_port,result_r.remote_host,result_r.remote_port,result_r.runLength,result_r.bytes,result_r.bps,
               result_r.hostUtilization,result_r.remoteUtilization,result_r.intervalLength))
