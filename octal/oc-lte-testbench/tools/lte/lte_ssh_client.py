from opentest.server.service.ssh_service import SSHServiceClient
try:
    from opentest.lte.websocket_api.ws_api import EPC_WS
    from opentest.lte.websocket_api.interfaces.LOCAL_INTERF import *
    from opentest.lte.iperf_tools.iperf_utils import Iperf_Launcher, Iperf_Parser
except:
    pass
import lte_calibtools
# from opentest.scripts import testrunner
import time
import subprocess
import lte_calibtools
import lte_comport
import re

error_string = "Terminate","Authentication failed"
class E2ESSHServiceClient(SSHServiceClient):

    def launch_service(self, context):

        self.ssh_ip_addr = context.E2E_SSH_IP_ADDR
        ssh_username = context.E2E_SSH_USERNAME
        ssh_password = context.E2E_SSH_PASSWORD

        self.e2e = Iperf_Launcher(self)
        self.result = Iperf_Parser()

        context.logger.info("Waiting end of boot and launching SSH connection")
        while True:
            try:
                self.create_service(self.ssh_ip_addr, ssh_username, ssh_password, timeout=2)
                break
            except testrunner.TesterInterruption:
                raise
            except Exception as e:
                print e
                if self.error_string_handle(error_string, str(e)):
                    time.sleep(1)
                    pass
                else:
                    raise

    def mme_setup(self):
        # Will launch MME if it do not exist.
        if not self.mme_connected():
            self.execute_command("sudo nohup bash /root/mme/lte_start.sh", stdin=["nuran"], get_pty=True)

    def mme_connected(self):
        try :
            mme_interf = LOCAL_INTERF(self.ssh_ip_addr,"9000")

            #Instantiate MME object using the local websocket interface
            self.mme1 = EPC_WS(mme_interf)

            #Initiate connection to the MME
            self.mme1.connect()
            return True
        except Exception as e:
            self.logger.info(e)
            return False

    def ue_connected(self):
        if self.mme1.ue_get() == []:
            return False
        else:
            return True

    def enb_connected(self):
        if  self.mme1.enb() == []:
            return False
        else:
            return True

    def iperf_launch_server_remote(self, logfile, iperf_script="iperf_server.sh", arguments=[]):
        self.e2e.iperf_launch_server_remote(password='nuran', iperf_script_path=iperf_script, arguments=arguments, logfile=logfile)

    def iperf_run_client_local(self, logfile, target_ip='192.168.2.1', arguments=[] ):
        self.e2e.iperf_run_client_local(target_ip=target_ip, arguments=arguments, logfile=logfile)

    def iperf_run_client_local_reverse(self, logfile, target_ip='192.168.2.1', arguments=[]):
        self.e2e.iperf_run_client_local_reverse(target_ip=target_ip, arguments=arguments, logfile=logfile)

    def iperf_killremote(self):
        self.e2e.iperf_killremote('nuran')

    def extract(self, file):
        self.result.extract_from_file(file=file)
        return self.result

    def bps(self):
        return self.result.__string__()

    def iperf_s(self):
        # start server in VM
        return self.execute_command("iperf -s -i 10")

    def iperf_c(self):

        return subprocess.check_output([r'C:\GIT_Files\PicoLTE-TB\tools\iperf\support\iperf-2.0.9-win64\iperf.exe', '-c', '192.168.2.1', '-r'])

    def error_string_handle(self, error_array, string):
        for i in range(0, len(error_array)):
            if error_array[i] in string:
                return False
        return True

class LTESSHServiceClient(SSHServiceClient):

    def read_board_rev(self):
        return self.execute_command('fe-gethdrev')

    def wait_for_initializing(self, context):
        ssh_ip_addr = context.SSH_IP_ADDR
        ssh_username = context.SSH_USERNAME
        ssh_password = context.SSH_PASSWORD
        # Will Try to execute create_service ssh until device is ready
        timeout = 0
        self.logger.info("Waiting end of boot and launching SSH connection")
        while True:
            try:
                self.create_service(ssh_ip_addr, ssh_username, ssh_password, timeout=2)
                break
            except Exception as e:
                if self.error_string_handle(error_string, str(e)):
                    time.sleep(1)
                    pass
                else:
                    raise
        self.logger.info("Waiting end of initialization")
        while not self.is_done_initializing():
            time.sleep(2)

    def reboot(self):
        self.execute_command('reboot', timeout=1, assertexitcode=-1)
        self.logger.info('Rebooting...')

    def is_done_initializing(self):
        boot_messages = self.execute_command('cat /var/log/messages')
        return '/sbin/getty -L ttyS0 115200 vt100' in boot_messages

    def error_string_handle(self, error_array, string):
        for i in range(0, len(error_array)):
            if error_array[i] in string:
                return False
        return True

default = "subsystem", "modalias", "name", "uevent"
TEST_SUCCESS_STRING = "test pass"
class I2CDetectSSHServiceClient(SSHServiceClient):

    def find(self, path ,expected_addr):
        missing = ""
        cmd = "ls " + path

        found = self.execute_command(cmd)
        for address in expected_addr:
            if address not in found:
                missing = missing + address + " "
        return missing

    def detect(self, path ,expected_addr):
        import re
        regex = re.compile('[\s\S]*\dm([.\w]+)[\s\S]*')
        fail = ""
        for addr in expected_addr:
            success = False
            cmd = "ls " + path+"/"+path[-1:]+"-00" +addr[-2:]
            found = self.execute_command(cmd)
            #print repr(found)
            if "No such file or directory" in found.strip():
                success = False
            else :
                for fil in found.split():
                    match = regex.findall(fil)
                    if match != []:
                        fil= match[0]
                        if fil not in default:
                            success = True
                    elif fil not in default:
                        success = True
            if success == False:
                fail = fail + addr + " "
        if fail == "":
            return TEST_SUCCESS_STRING
        else :
            return ("Missing : " + fail)

command = {"gpslock" : "sh /usr/bin/gpsGetLoc.sh",
           "dummy" : "/"}
class GPSSSHServiceClient(SSHServiceClient):

    def lock(self, first_time):
        string = ""
        i = 0
        if first_time:
            timeout = 300
        else:
            timeout = 200
        cmd = command["gpslock"]
        string= self.execute_command(cmd, timeout=timeout, assertexitcode=1)

        return (string)

    def pps_bool(self):
        success = False
        i = 0
        self.execute_command("echo 5 >  /sys/class/gpio/export", assertexitcode=None)
        value = self.execute_command("cat /sys/class/gpio/gpio5/value", assertexitcode=None)
        check = value
        while check == value and i < 30:
            value = self.execute_command("cat /sys/class/gpio/gpio5/value", assertexitcode=None)
            i = i + 1
        if i < 10:
            success = True

        return success

class LEDSSHServiceClient(SSHServiceClient):

    def control(self, color="orange", on_off="on"):
        remote_path = lte_calibtools.REMOTE_CALIBRATION_PATH
        # color : ["red", "green" or "orange"] | on_off: ["on" or "off"]
        cmd ="fe-setled "+color+" "+on_off
        self.execute_command(cmd, assertexitcode=None)

sensorpath = {"tx0_temp":"/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-004a/",
            "tx1_temp":"/sys/devices/soc.0/1180000001200.i2c/i2c-1/1-004a/",
            "fe_temp":"/sys/devices/soc.0/1180000001200.i2c/i2c-1/1-0049/",
            "bb_temp":"/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0048/",
            "tx0_current":"/sys/devices/soc.0/1180000001200.i2c/i2c-1/1-0040/",
            "tx1_current":"/sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0040/"}

temp_info = ["temp1_input", "temp1_max", "temp1_max_hyst"]
curr_info = ["curr1_input","power1_input", "in1_input"]

alarm ={"bb_temp":"1",
        "fe_temp":"fe_temperature_sensor_alert",
        "fe_current":"fe_current_sensor_alert"}
class SensorSSHServiceClient(SSHServiceClient):

    def temp_tx(self, tx):#tx take 1 or 0 as argument (int)
        sensor = ("tx" + str(tx) + "_temp")
        value = self.read(sensorpath[sensor]+temp_info[0])
        value = float(value)/1000.0
        self.logger.debug("TX%d PA Temp = %.2f C" % (lte_calibtools.real_tx(tx), value))
        return value

    def max_temp_tx(self, tx):#tx take 1 or 0 as argument (int)
        sensor = ("tx" + str(tx) + "_temp")
        value = self.read(sensorpath[sensor]+temp_info[1])
        return float(value)/1000.0

    def hyst_temp_tx(self, tx):#tx take 1 or 0 as argument (int)
        sensor = ("tx" + str(tx) + "_temp")
        value = self.read(sensorpath[sensor]+temp_info[2])
        return float(value)/1000.0

    def current_tx(self, tx):#tx take 1 or 0 as argument (int)
        # sensor = ("tx" + str(tx) + "_current")
        # value = self.read(sensorpath[sensor]+curr_info[0])
        value = self.execute_command('fe-getcurrent pa%d' % (tx+1)).strip()
        return float(value)/float(1000)

    def power_tx(self, tx):#tx take 1 or 0 as argument (int)
        sensor = ("tx" + str(tx) + "_current")
        value = self.read(sensorpath[sensor]+curr_info[1])
        return float(value)/float(1000)

    def voltage_tx(self, tx):#tx take 1 or 0 as argument (int)
        sensor = ("tx" + str(tx) + "_current")
        try:
            value = self.read(sensorpath[sensor]+curr_info[2])
        except:
            value = 28000
        return float(value)/float(1000)

    def temp_sys(self, sys):#sys take "fe" or "bb" as argument (string)
        sensor = (sys + "_temp")
        value = self.read(sensorpath[sensor]+temp_info[0])
        return float(value)/float(1000)

    def max_temp_sys(self, sys):#sys take "fe" or "bb" as argument (string)
        sensor = (sys + "_temp")
        value = self.read(sensorpath[sensor]+temp_info[1])
        return value

    def hyst_temp_sys(self, sys):#sys take "fe" or "bb" as argument (string)
        sensor = (sys + "_temp")
        value = self.read(sensorpath[sensor]+temp_info[2])
        return value

    def read_info(self, sensor):# sensor : "tx0_temp","tx1_temp,"fe_temp","bb_temp","tx0_current","tx1_current"
        string =""
        if "temp" in sensor:
            for i in range(0,len(temp_info)):
                value = self.read_file(sensorpath[sensor]+temp_info[i]).strip()
                valueclean = (str(int(value)/1000) +"."+ str(int(value)%1000))
                string = (string + sensor +" "+ temp_info[i] +" : " + valueclean + "\n")
        if "current" in sensor:
            for i in range(0,len(curr_info)):
                value = self.read_file(sensorpath[sensor]+curr_info[i]).strip()
                valueclean = (str(int(value)/1000) +"."+ str(int(value)%1000))
                string = (string + sensor +" "+ curr_info[i] +" : " + valueclean + "\n")
        return (string)

    def read_all(self):
        read = ["tx0_temp","tx1_temp","fe_temp","tx0_current","tx1_current"]#TODO add "bb_temp" when revb is ok
        string =""
        for i in range(0,len(read)):
            temp_string=self.read_info(read[i])
            string = string + temp_string +"\n"
        return (string)

    def read(self, path):
        value = self.read_file(path).strip()
        return (value)

    def _read_threshold(self, sensor):
        cmd = "cat "+sensorpath[sensor]+"temp1_max"
        reset_max = self.execute_command(cmd).strip()
        cmd = "cat "+sensorpath[sensor]+"temp1_max_hyst"
        reset_hyst = self.execute_command(cmd).strip()
        return reset_max, reset_hyst

    def _set_threshold(self, sensor, threshold_max, threshold_hyst):
        cmd = "echo "+sensorpath+" > "+sensorpath[sensor]+"temp1_max"
        self.execute_command(cmd)
        cmd = "echo "+sensorpath+" > "+sensorpath[sensor]+"temp1_max_hyst"
        self.execute_command(cmd)

    def tempalert_bool(self, sensor):
        success = False
        reset_max, reset_hyst = self._read_threshold(sensor=sensor)
        alarm_off = self._alarm_temp_fe()
        self._set_threshold(sensor, "10000", "5000")
        time.sleep(0.5)
        if self._alarm_temp_fe() != alarm_off :
            success = True
        self._set_threshold(sensor, reset_max, reset_hyst)
        time.sleep(0.5)
        return (sucess)

    def _alarm_temp_fe(self):
        self.execute_command("echo 13 > /sys/class/gpio/export")
        value = self.execute_command("cat /sys/class/gpio/gpio13/value")
        return(value)

    def alarm_alert_bool(self, sensor):# sensor : "tx0_temp","tx1_temp,"fe_temp","bb_temp","tx0_current","tx1_current"
        success = False
        alarm_code = "fe_temp"
        if "temp" in sensor:
            reset_max, reset_hyst = self._read_threshold(sensor=sensor)
            offalarm = str(self._alarm(alarm_code))
            self._set_threshold(sensor, "10000", "5000")

        if "current" in sensor:
            alarm_code = "fe_current"
            #TODO no setable threshold for current available.

        if "bb" in sensor:
            alarm_code = "bb_temp"

        time.sleep(2)
        if str(self._alarm(alarm_code)) != offalarm :
            success = True

        if alarm_code == "fe_current":
            pass
            #TODO no setable threshold for current available.
        else :
            self._set_threshold(sensor, reset_max, reset_hyst)

        time.sleep(0.5)
        return (success)

    def _alarm(self, sensor):#Read alarm (return 0 if trigger)
        if sensor == "bb_temp":
            self.execute_command("echo "+alarm[sensor]+" > /sys/class/gpio/export")
            value = self.execute_command("cat /sys/class/gpio/gpio"+alarm[sensor]+"/value")
        else:
            value = self.execute_command("cat /sys/devices/soc.0/0.tip/"+alarm[sensor])
        return(value)

class SDCardSSHServiceClient(SSHServiceClient): #used to be drive.py

    def mount(self, device, path, succes):
        #this command create a new direcectory (/tmp/test) that will be mount by the sd card
        path_folder_test = '{}/test'.format(path)
        cmd = 'mkdir {}'.format(path_folder_test)
        try:
            self.execute_command(cmd, assertexitcode=None)
        except:
            #Unable to create directory, maybe it exists
            pass

        #mount the sd card on the new directory
        cmd = 'mount -t vfat {} {}'.format(device,path_folder_test)
        resp = self.execute_command(cmd, assertexitcode=None).strip()

        if resp != '' :
            succes = False

        #return the directory
        return path_folder_test, succes

    def validate(self, path_folder_test, succes):
        #file_path should be /tmp/test/driver_test_file at this point
        content = 'test32123'
        file_name = "/driver_test_file"
        path_file_test = "{}{}".format(path_folder_test,file_name)

        #file is created here
        cmd = "touch {}".format(path_file_test)
        self.execute_command(cmd, assertexitcode=None)

        #this command writes in the new file
        self.write_file(path_file_test, content)

        #this command reads in the new file
        response = self.read_file(path_file_test).strip()

        #succes will equal to 1 if the the content written and read is the same and if the file is cleared correctly
        if response != content:
            succes = False

        return succes, path_folder_test, path_file_test

    def unmount(self, device, path_folder_test, path_file_test, succes, path):
        content = 'test32123'

        #unmount the sd card
        cmd = "umount {}".format(device)
        self.execute_command(cmd, assertexitcode=None)

        #test if unmount worked
        resp = self.read_file(path_file_test)

        if resp == content:
            succes = False

        #mount to delete the temporary directory named test
        cmd = 'mount -t vfat {} {}'.format(device, path_folder_test)
        self.execute_command(cmd, assertexitcode=None)
        cmd_rm = 'rm {}'.format(path_file_test)
        self.execute_command(cmd_rm, assertexitcode=None)

        #check if the file is remove correctly from the sd card
        cmd = 'ls {}'.format(path_folder_test)
        response = str(self.execute_command(cmd, assertexitcode=None))
        resp = response.find("driver_test_file")
        if resp != -1:
            succes = False

        #unount sd card to close the test and remove the folder /temp/test
        cmd = "umount {}".format(device)
        self.execute_command(cmd, assertexitcode=None)
        cmd_rm = 'rm -r {}'.format(path_folder_test)
        self.execute_command(cmd_rm, assertexitcode=None)

        #check if the folder is remove correctly from the bb
        cmd = 'ls {}'.format(path)
        response = str(self.execute_command(cmd, assertexitcode=None))
        resp = response.find("test")
        if resp != -1:
            succes = False

        return succes

    def test(self, device, path):
        succes = True
        path_folder_test, succes =self.mount(device, path, succes)
        succes, path_folder_test, path_file_test = self.validate(path_folder_test,succes)
        succes = self.unmount(device, path_folder_test,path_file_test, succes, path)
        return succes
RMS_DICT = {'trx1_fwd':"fe-getreversvalue -t 2 -c A",
            'trx1_rev':"fe-getreversvalue -t 2 -c B",
            'trx0_fwd':"fe-getreversvalue -t 1 -c A",
            'trx0_rev':"fe-getreversvalue -t 1 -c B"}
class RMSSSHServiceClient(SSHServiceClient):

    def read_all(self):
        """
            This function reads the forward and the reverse value of the specified trx
            Then it prints the value
            The function raises and exception error if the device is not found or does not exist
        """

        content_1 = self.read_rms('trx0_fwd').strip()
        content_2 = self.read_rms('trx0_rev').strip()
        content_3 = self.read_rms('trx1_fwd').strip()
        content_4 = self.read_rms('trx1_rev').strip()

        return content_1,content_2,content_3,content_4


    def read_rms(self, device):
        """
            This function reads the forward and the reverse value of the specified trx
            Then it prints the value
            The function raises and exception error if the device is not found or does not exist
        """

        if device in RMS_DICT:
            content = self.execute_command(RMS_DICT[device]).strip()
            return float(content)
        else:
            raise Exception("Device does not exist : %s" % device)

path_wp_eeprom_calib = "/sys/devices/soc.0/0.tip/eeprom_tiva_wp"
path_eeprom_0050 = "/sys/bus/i2c/devices/0-0050/eeprom"
class EEPROMSSHServiceClient(SSHServiceClient):

    def get_calib_wp(self):
        """
            This function reads the write protection file for the calibration EEPROM and prints it.
            It does the same thing for the calibration EEPROM file.
            If the file is not found, the function does nothing
        """
        result = self.read_file(path_wp_eeprom_calib)
        return(result)

    def set_calib_wp(self, content):
        """
            This function checks if the content is 0 or 1 and overwrites it in the file (enable)
            If the content is not 0 or 1, the function raises an exception
        """
        if (content == '0' or content == '1' or content == 0 or content == 1):
            self.write_file(path_wp_eeprom_calib, content)
        else:
            raise Exception("Content value is not available : %s" % content)

    def eeprom_test(self):
        succes = False
        original_wp = self.get_calib_wp()
        original_wp = str(original_wp[:1])
        original_path,emptyfile_path,newfile_path = "/tmp/og_path","/tmp/ef_path","/tmp/nf_path"

        self.set_calib_wp('1')
        self.execute_command("touch {};touch {};touch {}".format(original_path,emptyfile_path,newfile_path))
        byte = "00"
        self.execute_command("echo {} > /tmp/ef_path".format(byte))
        self.execute_command("dd if={} of={} bs=1 count=2".format(path_eeprom_0050,original_path))

        #put empty spaces in eeprom and put the empty spaces in another file
        self.execute_command("dd if={} of={} bs=1 count=2".format(emptyfile_path,path_eeprom_0050))
        self.execute_command("dd if={} of={} bs=1 count=2".format(path_eeprom_0050,newfile_path))

        #compare the two files
        resp1 = str(self.read_file(emptyfile_path))
        resp2 = str(self.read_file(newfile_path))
        resp1 = resp1.strip()
        resp2 = resp2.strip()
        if resp1 == resp2:
            succes = True

        #set the value to what they were originally
        self.execute_command("dd if={} of={} bs=1 count=2".format(original_path,path_eeprom_0050))
        self.execute_command("rm {};rm {};rm {}".format(original_path,emptyfile_path,newfile_path))
        self.set_calib_wp(original_wp)

        return succes
import re
class EnvVarSSHServiceClient(SSHServiceClient):
    VALUE_REGEX = re.compile(r"(?:Value= '(\w+)')")

    def get_value(self, var_name):
        return_value = self.execute_command('fprintenv %s' % var_name)
        match = EnvVarSSHServiceClient.VALUE_REGEX.findall(return_value)
        if match:
            return match[0]
        else:
            raise Exception("Unable to extract EnvVar value from %s. Returned string: %s" % (var_name, return_value))

    def set_value(self, var_name, value):
        self.logger.debug('%s=%s' % (var_name, value))
        self.execute_command("fsetenv %s '%s'" % (var_name, value))

    def setup_from_file(self, file_name):
        env_vars = lte_comport.parse_env_file(file_name)
        self.setup_from_dict(env_vars)

    def setup_from_dict(self, var_dict):
        for envvar, value in var_dict.iteritems():
            self.set_value(envvar, value)

    # move from flash to tftp (min reverse) order (tftp, flash)
    def setup_from_file_and_in_second(self, setup_file, in_second):
        env_vars_setup = lte_comport.parse_env_file(setup_file)
        env_vars_present = lte_comport.parse_env_file(in_second)

        for envvar, value in env_vars_setup.iteritems():
            if envvar in env_vars_present:
                self.set_value(envvar, value)

    # pass them in order, precombine, do one write
    def clear_all_and_setup_from_files(self, *files):
        EXCLUDE_VARS = ['ethaddr']
        all_env_vars = {}
        for env_var_file in files:
            all_env_vars.update(lte_comport.parse_env_file(env_var_file))

        current_env_vars = self.read_all_env_vars()

        # All variables which are currently present but that will not be overwritten
        # by new variables are assigned to an empty string to be erased
        for current_var in current_env_vars:
            if current_var not in all_env_vars and current_var not in EXCLUDE_VARS:
                all_env_vars[current_var] = ''

        self.setup_from_dict(all_env_vars)



    def read_all_env_vars(self):
        env_vars = {}
        regex =r'([\w]+)=([\S\ \t]*)\n'

        content = self.execute_command('fprintenv')

        matches = re.findall(regex, content)
        for match in matches:
            env_vars[match[0]] = match[1]

        return env_vars
