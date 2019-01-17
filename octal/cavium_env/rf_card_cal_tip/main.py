#!/usr/bin/python

import os
import sys
import time
import common
import threading
import cal_rx_rssi
import cal_tx_pwr
import cal_ref_clk_dac
import cal_ref_clk_pwm
import cal_ref_clk_dax
import cal_freq_err_pwm
import test_tx_evm
import test_tx_cw
import test_flatness
import test_tx_cfr
import cal_iq_offset
import test_config
import enodeb_ctrl
import eutra_bands
import eutra_earfcn
import im_calibration
import test_rx_sensitivity
import cal_iq_offset_ms8870a
import cal_iq_offset_cmw500
import bb_testterm
import bb_eeprom
#from subprocess import Popen, PIPE, STDOUT

if os.name == "nt":
    import msvcrt
else:
    from select import select

def input_with_timeout_sane(prompt, timeout, default):
    """Read an input from the user or timeout"""
    print prompt,
    sys.stdout.flush()
    rlist, _, _ = select([sys.stdin], [], [], timeout)
    if rlist:
        s = sys.stdin.readline().replace('\n','')
    else:
        s = default
        print s
    return s

# msvcrt.getche not working in eclipse
def input_with_timeout_windows(prompt, timeout, default): 
    
    start_time = time.time()
    print prompt,
    sys.stdout.flush()
    input = ''
    read_f = msvcrt.getche
    input_check = msvcrt.kbhit
    
    if not sys.stdin.isatty( ):
        read_f = lambda:sys.stdin.read(1)
        input_check = lambda:True
    
    print 'time=' + str(time.time()) + ', start=' + str(start_time)
    while True:
        if input_check():
            chr_or_str = read_f()
            try:
                if ord(chr_or_str) == 13: # enter_key
                    break
                elif ord(chr_or_str) >= 32: #space_char
                    input += chr_or_str
            except:
                input = chr_or_str
                break #read line,not char...    
                
        if len(input) == 0 and (time.time() - start_time) > timeout:
            break
        
    if len(input) > 0:
        return input
    else:
        return default

class rfCardCal():
    
    def __init__(self):
        
        self.version = '2015.10.06'
        self.dir_report = 'test_report'
        self.default_sn = 'test'
        self.mod = ''
        self.earfcn_dl = 0
        self.num_rec = 0
        self.curr_rec_num = 0
        self.ee_records = []
        self.rec_exists = False
        self.start_time = 0
        self.elapsed_time = 0
        self.report_name = ''
        self.report_hndl = 0
        self.retest_num = 3
        self.telnet_retry = 20
        self.cfg = test_config.EnbConfig()
        if self.cfg.read_cal_channel() < 0 : sys.exit()
        self.enb = enodeb_ctrl.enodeB_Ctrl()
    
    def create_new_eeprom_record(self):
        
        self.enb.enb_cd_usr_bin()
        
        if (int(self.num_rec) == 0) or (int(self.num_rec) == 255):
            self.enb.enb_eeprom_edit_header('wsn', self.sn)
            self.enb.enb_eeprom_edit_header('wbt', str(self.cfg.eeprom_board_type))
            self.enb.enb_eeprom_edit_header('wrt', str(self.cfg.eeprom_rfic_type))
        
        if (int(self.curr_rec_num) == 1) and \
            ((int(self.num_rec) > 0) and (int(self.num_rec) < 255)):
            pass
        else:
            self.enb.enb_eeprom_edit_rec_num('wnr', int(self.num_rec) + 1)
        
        self.enb.enb_eeprom_edit_record('wrv', self.curr_rec_num, '1') # record version
        time.sleep(0.5)
        print "write EEPROM EARFCN = " + str(self.earfcn_dl)
        self.enb.enb_eeprom_edit_record('wed', self.curr_rec_num, str(self.earfcn_dl))
        time.sleep(0.5)
        
        self.enb.enb_eeprom_edit_record('wtc', self.curr_rec_num, str(self.cfg.eeprom_tcxo_ctrl))
        
        #print "write EEPROM gain prim = " + str(self.cfg.gain1)
        self.enb.enb_eeprom_edit_record('wrgp', self.curr_rec_num, str(self.cfg.gain1))
        #print "write EEPROM gain sec = " + str(self.cfg.gain2)
        self.enb.enb_eeprom_edit_record('wrgs', self.curr_rec_num, str(self.cfg.gain2))
        #print "write EEPROM attn prim = " + str(self.cfg.attn1)
        self.enb.enb_eeprom_edit_record('wtap', self.curr_rec_num, str(self.cfg.attn1))
        #print "write EEPROM attn sec = " + str(self.cfg.attn2)
        self.enb.enb_eeprom_edit_record('wtas', self.curr_rec_num, str(self.cfg.attn2))
        time.sleep(0.5)
                
    def open_test_report(self, mydir):
        
        if (self.cfg.test_report == True) and \
        ((self.cfg.board_typ == 'zen_ad') or (self.cfg.board_typ == 'zen_ak')):
            """
            if self.is_empty_eeprom() == False:
                self.enb.enb_cd_usr_bin()
                self.sn = self.enb.enb_eeprom_get_serial_num()
                self.sn = self.sn.strip()
                print "RF serial number: " + self.sn
            else:
                self.sn = raw_input("input RF card serial number: ")
            """
            
            self.sn = self.default_sn
            
            self.report_name = self.sn.strip() + '_' + time.strftime("%Y%m%d") + '_' + time.strftime("%H%M%S") + '.txt'
            self.report_name = './' + mydir + '/' + self.report_name
            
            self.report_hndl = open(self.report_name, "wb")
            self.report_hndl.write(self.report_name + '\n')
            self.report_hndl.write(time.strftime("%d/%m/%Y") + ' ' + time.strftime("%H:%M:%S") + '\n')
            #self.report_hndl.write('bandwidth = ' + str(self.cfg.cal_bandwidth) + '\n')
        
    def close_test_report(self):
        
        if self.cfg.test_report == True:
            self.report_hndl.close()
        
    def write_uboot_cal_variables(self):
        
        self.enb.enb_cd_usr_bin()
        
        """
        self.enb.wr_ubootenv_bw(str(self.cfg.cal_bandwidth))
        self.enb.wr_ubootenv_freq(str(test_config.dl_freq), \
                                  str(test_config.ul_freq))
        self.enb.wr_ubootenv_gain(str(self.cfg.gain1), str(self.cfg.gain2))
        self.enb.wr_ubootenv_atten(str(self.cfg.attn1), str(self.cfg.attn2))
        """
        """
        self.enb.tn_write(im_calibration.pp_base, "fsetenv mode pltd", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv bootby tftp", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv sebypass 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv startapp 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv ptpenable 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv gpsenable 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv swloadby tftp", 3)
        """
        print "set calibration variables..."
        self.enb.tn_write(im_calibration.pp_base, "fsetenv mk_ubootenv 1", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv bw " \
                          + str(self.cfg.cal_bandwidth), 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv gain1 " \
                          + str(self.cfg.gain1), 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv gain2 " \
                          + str(self.cfg.gain2), 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv atten1 " \
                          + str(self.cfg.attn1), 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv atten2 " \
                          + str(self.cfg.attn2), 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv txfreq " \
                          + str(test_config.dl_freq), 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv rxfreq " \
                          + str(test_config.ul_freq), 3)
        #self.enb.tn_write(im_calibration.pp_base, "fsetenv pwmreghigh 23456", 3)
        #self.enb.tn_write(im_calibration.pp_base, "fsetenv pwmreglow 37376", 3)
        
        #self.enb.tn_write(im_calibration.pp_base, "fsetenv logdispen 2", 3)
        #self.enb.tn_write(im_calibration.pp_base, "fsetenv logdispport " \
        #                  + str(self.cfg.my_udp_port), 3)
        #self.enb.tn_write(im_calibration.pp_base, "fsetenv logparserip " \
        #                  + str(self.cfg.my_udp_ipaddr), 3)
        """
        self.enb.tn_write(im_calibration.pp_base, "fsetenv RSSI_OFFSET_PRIM 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv RSSI_OFFSET_SEC 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv RSSI_SLOPE_PRIM 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv RSSI_SLOPE_SEC 0", 3)
        
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX1_I_OFFSET 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX1_Q_OFFSET 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX2_I_OFFSET 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX2_Q_OFFSET 0", 3)
        
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX_OFFSET_PRIM 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX_OFFSET_SEC 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX_SLOPE_PRIM 0", 3)
        self.enb.tn_write(im_calibration.pp_base, "fsetenv TX_SLOPE_SEC 0", 3)
        """
    def write_temp_ox_slope(self):
        
        if (test_config.band == 7) or (test_config.band == 38):
            self.enb.enb_eeprom_edit_record('wtsx', self.curr_rec_num, "2.5")
        elif (test_config.band == 4):
            self.enb.enb_eeprom_edit_record('wtsx', self.curr_rec_num, "5")
        else:
            self.enb.enb_eeprom_edit_record('wtsx', self.curr_rec_num, "2.5")
    
    def write_reserved_eeprom_data(self):
        
        self.enb.enb_eeprom_edit_record('wtst', self.curr_rec_num, "0")
        self.enb.enb_eeprom_edit_record('wtsr', self.curr_rec_num, "0")
        self.enb.enb_eeprom_edit_record('wrtr', self.curr_rec_num, "0")
        self.enb.enb_eeprom_edit_record('wtm', self.curr_rec_num, "0")
            
    def calibration_all(self):
        
        self.start_time = time.time()
        
        if (self.cfg.manual_switch_instr == True):
            if (self.cfg.test_set == "agilent"):
                common.hit_continue("Connect cable to EXG")
            else:
                common.hit_continue("Connect cable to MT8870A output port")
        
        # RSSI calibration
        om = cal_rx_rssi.CalRxRssi(self.report_hndl, self.curr_rec_num)
        om.enb.start_telnet_session()
        om.run()
        om.enb.end_telnet_session()
        del om
        
        if (self.cfg.manual_switch_instr == True):
            if (self.cfg.test_set == "agilent"):
                common.hit_continue("Connect cable to MXA")
            else:
                common.hit_continue("Connect cable to MT8807A input port")
        else:
            time.sleep(2)   # avoid system exception error
        
        # Reference clock calibration
        if (self.cfg.test_set == "agilent"):
            if (self.cfg.board_typ == "refkit1"):
                om = cal_ref_clk_dac.CalRefClk(self.report_hndl, self.curr_rec_num)
            elif (self.cfg.board_typ == "zen_ad"):
                if (self.cfg.tcxo_ctrl == "pwm"):
                    om = cal_ref_clk_pwm.CalRefClk(self.report_hndl, self.curr_rec_num)
                elif (self.cfg.tcxo_ctrl == "dax"):
                    om = cal_ref_clk_dax.CalRefClkDax(self.report_hndl, self.curr_rec_num)
            else:
                print "board not supported"
        else:
            om = cal_freq_err_pwm.CalFreqErr(self.report_hndl, self.curr_rec_num)
        
        om.enb.start_telnet_session()
        om.run()
        om.enb.end_telnet_session()
        del om
        
        # IQ DC offset calibration
        if (self.cfg.test_set == 'agilent'):
            om = cal_iq_offset.CalIQOffset(self.report_hndl, self.curr_rec_num)
        elif (self.cfg.test_set == 'anritsu'):
            om = cal_iq_offset_ms8870a.CalIQOffsetMT8870A(self.report_hndl, self.curr_rec_num)
        elif (self.cfg.test_set == 'rs'):
            om = cal_iq_offset_cmw500.CalIQOffsetCMW500(self.report_hndl, self.curr_rec_num)
        
        om.enb.start_telnet_session()
        #time.sleep(2) #(5)
        om.run()
        om.enb.end_telnet_session()
        
        # TX output power calibration
        om = cal_tx_pwr.CalTxPwr(self.report_hndl, self.curr_rec_num)
        om.enb.start_telnet_session()
        #time.sleep(2) #(5)
        om.run()
        om.enb.end_telnet_session()
        
        # TX EVM optimization
        om = test_tx_evm.TestTxEvm(self.report_hndl, self.curr_rec_num)
        om.enb.start_telnet_session()
        #time.sleep(2) #(5)
        om.run()
        
        # System reboot
        om.enb.enb_reboot_in_dsp_run()
        print "rebooting enodeB..."
        im_calibration.is_dsp_running = False
        time.sleep(2)
        om.enb.end_telnet_session()
        del om
        
        if self.cfg.do_sens_test_in_opt1 == True:
            time.sleep(10)
            om = test_rx_sensitivity.RxSens(self.report_hndl)
            
            for cnt in range(self.telnet_retry):
                if (om.enb.start_telnet_session() == -1):
                    #print "retry connect, count " + str(cnt + 1)
                    time.sleep(5) #(10)
                else:
                    break
                if (cnt == (self.telnet_retry - 1)):
                    print "telnet connection fail"
                    sys.exit()
        
        # RX sensitivity test
        if (self.cfg.do_sens_test_in_opt1 == True):
            
            time.sleep(20)
            if (self.cfg.manual_switch_instr == True):
                if (self.cfg.test_set == "agilent"):
                    common.hit_continue("Connect cable to EXG")
                else:
                    common.hit_continue("Connect cable to MT8870A output port")
            
            #om = test_rx_sensitivity.RxSens(self.report_hndl)
            
            om.enb.start_telnet_session() # for testing
            
            time.sleep(20) #5
            om.run_limit_test()
            #om.run()
            
            # System reboot
            om.enb.enb_reboot_in_dsp_run2()
            print "rebooting enodeB..."
            im_calibration.is_dsp_running = False
            time.sleep(2)
        
            om.enb.end_telnet_session()
            #del om
            time.sleep(5) #(20)
        
        self.elapsed_time = float(time.time() - self.start_time)
        print "End of calibration test"
        print "Elapsed time = %.2f [sec]" % self.elapsed_time
        print ""
        
    def load_original_environment(self):
        
        self.enb.get_macaddr()
        self.enb.enb_cd_usr_bin()
        self.write_uboot_cal_variables()
        
    def is_ascii(self, mystring):
        
        try:
            mystring.decode('ascii')
        except UnicodeDecodeError:
            return False
        else:
            return True
        
    def do_bb_eeprom_record_not_found(self, om):
        
        print ''
        print 'baseboard serial number no found'
        sn = raw_input("enter a serial number:")
        om.enb.enb_bb_eeprom_edit_record('wsn', sn)
        
        answer = raw_input("write baseboard parameters from bb_eeprom?(y or n):")
        if (answer == 'y') or (answer == 'Y'):
            
            print 'write data into baseboard...'
            om.write_bb_eeprom()
        else:
            print 'skip writing other baseboard data'
        
    def check_bb_eeprom_record(self):
        
        self.enb.end_telnet_session()
        time.sleep(2)
        om = bb_eeprom.BBEepromAccess()
        om.enb.start_telnet_session()
        om.start_enodeb()
        bbSerialNum = om.enb.enb_bb_eeprom_get_serial_number()
        
        if len(bbSerialNum) == 0:
            self.do_bb_eeprom_record_not_found(om)
        elif self.is_ascii(bbSerialNum[0]):
            print 'baseboard serial number: ' + bbSerialNum
        else:
            self.do_bb_eeprom_record_not_found(om)
        
        om.enb.end_telnet_session()
        time.sleep(2)
        self.enb.start_telnet_session()
        self.enb.enb_login()
        
    def get_eeprom_earfcn_list(self):
        """
        if self.cfg.en_eeprom_write == True:

            self.num_rec = int(self.enb.enb_eeprom_get_record_num())
            
            if (self.num_rec > 0) and (self.num_rec < 255):
                return self.enb.enb_eeprom_get_earfcn_dl()
        """
        pass
        

    def set_test_equipment(self):
        correct_ipaddr = 'n'
        while (correct_ipaddr != 'y'):
            print("\nSelected exg ipaddr = " + self.cfg.exg_ipaddr)
            correct_ipaddr = raw_input("Is this the correct exg ipaddr?(y/n):")
            if (correct_ipaddr == 'y') or (correct_ipaddr == 'Y'):
                break
            else:
                self.cfg.exg_ipaddr = raw_input("Please enter the correct exg ipaddr:")

        correct_ipaddr = 'n'
        while (correct_ipaddr != 'y'):
            print("\nSelected mxa ipaddr = " + self.cfg.mxa_ipaddr)
            correct_ipaddr = raw_input("Is this the correct mxa ipaddr?(y/n):")
            if (correct_ipaddr == 'y') or (correct_ipaddr == 'Y'):
                break
            else:
                self.cfg.mxa_ipaddr = raw_input("Please enter the correct mxa ipaddr:")

    def set_initial_frequency(self):

        if (self.cfg.cal_freq_arr[0][0] == 0) and (self.cfg.cal_freq_arr[0][1] == 0):
            mb = eutra_bands.BandFactory.newBand(test_config.band)
            test_config.dl_freq = mb.dl_freq()[1]
            test_config.ul_freq = mb.ul_freq()[1]
        else:
            test_config.dl_freq = self.cfg.cal_freq_arr[0][0]
            test_config.ul_freq = self.cfg.cal_freq_arr[0][1]

        correct_freq = 'n'
        while (correct_freq != 'y'):
            print("\nSelected DL freq = " + str(test_config.dl_freq))
            correct_freq = raw_input("Is this the correct DL freq?(y/n):")
            if (correct_freq == 'y') or (correct_freq == 'Y'):
                break
            else:
                test_config.dl_freq = float(raw_input("Please enter the correct DL freq:"))

        correct_freq = 'n'
        while (correct_freq != 'y'):
            print("\nSelected UL freq = " + str(test_config.ul_freq))
            correct_freq = raw_input("Is this the correct UL freq?(y/n):")
            if (correct_freq == 'y') or (correct_freq == 'Y'):
                break
            else:
                test_config.ul_freq = float(raw_input("Please enter the correct UL freq:"))

        print "\nUsing the following configuration:"
        print "Band " + str(test_config.band)
        print "EXG ip addr " + self.cfg.exg_ipaddr
        print "MXA ip addr " + self.cfg.mxa_ipaddr
        print "DL freq. " + str(test_config.dl_freq) + " MHz"
        print "UL freq. " + str(test_config.ul_freq) + " MHz"

    def is_empty_eeprom(self):
        
        res = False
        if (self.num_rec == 0) or (self.num_rec == 255):
            res = True
        return res
        
    def is_dl_center_freq(self, band, dl_freq):
        
        res = False
        ee = eutra_earfcn.EutraEarfcn()
        earfcn_mid = ee.get_middle_dl_earfcn(test_config.band)
        self.earfcn_dl = ee.dl_freq2earfcn(band, dl_freq)
        print "current downlink EARFCN = " + str(self.earfcn_dl)
        
        if (earfcn_mid == self.earfcn_dl): res = True
        return res
        
    def get_dl_center_freq(self, band):
        
        ee = eutra_earfcn.EutraEarfcn()
        return ee.get_middle_dl_earfcn(band)
        
    def get_ul_center_freq(self, band):
        
        ee = eutra_earfcn.EutraEarfcn()
        return ee.get_middle_ul_earfcn(band)
        
    def is_earfcn_record_exists_in_eeprom(self, band, dl_freq):
        
        self.rec_exists = False
        rec_size = len(self.ee_records)
        ee = eutra_earfcn.EutraEarfcn()
        dl_earfcn = ee.dl_freq2earfcn(band, dl_freq)
        
        for cn in range(rec_size):
            if (int(self.ee_records[cn][1]) == dl_earfcn):
                self.rec_exists = True
                self.curr_rec_num = cn + 1
                break
        
        return self.rec_exists
                        
    def prepare_eeprom_record(self, band, dl_freq):
        
        if self.is_empty_eeprom() == True:
            
            # if not center channel, put a dummy record in 1st position
            self.earfcn_dl = self.get_dl_center_freq(band)
            self.curr_rec_num = 1
            self.create_new_eeprom_record()
            self.write_temp_ox_slope()
            self.write_reserved_eeprom_data()
            self.num_rec = 1
                
            if self.is_dl_center_freq(band, dl_freq) == False:
                # custom DL/UL frequency
                self.curr_rec_num = 2
                self.create_new_eeprom_record()
                self.write_temp_ox_slope()
                self.write_reserved_eeprom_data()
        else:
            
            if self.is_earfcn_record_exists_in_eeprom(band, dl_freq) == False:
                
                if self.is_dl_center_freq(band, dl_freq) == True:
                    self.curr_rec_num = 1
                    print "record one"
                else:   # custom DL/UL frequency
                    self.curr_rec_num = int(self.num_rec) + 1
                    print "create a new EEPROM record"
                    self.create_new_eeprom_record()
                    self.write_temp_ox_slope()
                    self.write_reserved_eeprom_data()
            else:
                # overwrite record
                print "existing earfcn record"
        
    def find_eeprom_record(self):
        
        self.enb.start_telnet_session()
        self.enb.enb_login()
        self.enb.enb_cd_usr_bin()
        self.ee_records = self.get_eeprom_earfcn_list()
        self.prepare_eeprom_record(test_config.band, test_config.dl_freq)
        self.enb.end_telnet_session()
    
        
    def reboot_in_main_loop(self):
        
        self.enb.enb_reboot_in_dsp_run()
        time.sleep(2)
        self.enb.end_telnet_session()
        time.sleep(20)
        
        for cnt in range(self.telnet_retry):
            if (self.enb.start_telnet_session() == -1):
                #print "count " + str(cnt + 1)
                time.sleep(10)
            else:
                break
            if (cnt == (self.telnet_retry - 1)):
                print "telnet connection fail"
                sys.exit()
                
        self.enb.enb_login()
        
    def run(self):
        self.set_test_equipment()
        self.set_initial_frequency()  # for non-testall items
        self.enb.start_telnet_session()
        self.enb.enb_login()
        
        """
        # set up initial environment variables
        print ''
        prompt = 'set uboot from test_config?[y/n]:(n)'
        timeout = 5
        default = 'n'
        
        if os.name == 'posix':
            wr_init_uboot_var = input_with_timeout_sane(prompt, timeout, default)
        else:
            ##wr_init_uboot_var = input_with_timeout_windows(prompt, timeout, default)
            wr_init_uboot_var = raw_input("set uboot from test_config?(y/n):")
            
            #print prompt
            #time.sleep(1)
            #wr_init_uboot_var = 'y'
            
        if (wr_init_uboot_var == 'y') or (wr_init_uboot_var == 'Y'):
            print "\nwrite initial uboot variables\n"
            self.set_initial_frequency()
            self.load_original_environment()
            print "rebooting system..."
            self.reboot_in_main_loop()
        """ 
        # check baseboard EEPROM record
        #self.check_bb_eeprom_record()
            
        ## start calibraiton
        #self.ee_records = self.get_eeprom_earfcn_list()
        
        # create report
        if (not os.path.exists(self.dir_report)):
            print("create report directory: " + self.dir_report)
            os.system("mkdir " + self.dir_report)
        
        self.open_test_report(self.dir_report)
        self.enb.end_telnet_session()

        # connect instruments
        ic = im_calibration.Calibration(self.report_hndl, self.curr_rec_num)
        try:
            if (self.cfg.test_set == 'agilent'):
                print "connecting agilent test set"
                try:
                    print "connecting exg"
                    ic.exg_connect()
                    ic.exg_init()
                except:
                    print "not connected to exg"

                try:
                    print "connecting mxa"
                    ic.mxa_connect()
                    ic.mxa_init()
                    ic.mxa_setup(test_config.dl_freq)
                except:
                    print "not connected to mxa"
                
            elif (self.cfg.test_set == 'anritsu'):
                print "connecting anritsu test set"
                ic.mt8870a_connect()
                ic.mt8870a_init()
                
            elif (self.cfg.test_set == 'rs'):
                print "connecting rs test set"
                ic.cmw500_connect()
                ic.cmw500_init()
        except:
            print "test equipment is not connected"
        
        while (True):
            common.disp_test_title("Calibration Suite ver." + self.version)
            
            print("(1) Calibrate All Items")
            print("(2) RX RSSI Calibration")
            print("(3) Reference Clock Calibration")
            print("(4) IQ Offset Calibration")
            print("(5) TX Power Calibration")
            print("(6) TX Power Optimization")
            
            if (self.cfg.test_set == 'agilent'):
                print("(7) TX Transmit Test")
                print("(8) TX CW Test")
                
            print("(9) RX Sensitivity Test")
            print("(19) RX Continuous Test")
            print("(10)Flatness Test")
            print("(11)Baseboard Test")
            print("(12)Load RF EEPROM to uboot")
            print("(13)CFR Test")
            print("(21)Read Baseboard EEPROM Data")
            print("(22)Write Baseboard EEPROM Data")
            print("(23)Erase Baseboard EEPROM Data")
            print("(Q) Quit")
            
            self.mod = raw_input("Select Calibration Option:")
            
            if (self.mod == '1'):
                
                im_calibration.is_test_all = True
                    
                for ch in range(self.cfg.num_cal_channel):
                    # change flag for u-boot writing
                    if ch == 0:
                        test_config.wr_var_to_uboot = True
                    else:
                        test_config.wr_var_to_uboot = False
                    
                    # assign current test frequencies
                    if (ch != 0):
                        test_config.dl_freq = self.cfg.cal_freq_arr[ch][0]
                        test_config.ul_freq = self.cfg.cal_freq_arr[ch][1]
                    
                    self.report_hndl.write('\n*** Calibration #' + str(ch+1) + ' ***\n\n')
                    self.report_hndl.write('DL Freq.' + str(test_config.dl_freq) + ' MHz\n')
                    self.report_hndl.write('UL Freq.' + str(test_config.ul_freq) + ' MHz\n')
                    
                    # create or overwrite eeprom record
                    self.enb.start_telnet_session()
                    self.enb.enb_login()
                    self.load_original_environment()
                    self.enb.enb_cd_usr_bin()
                    self.ee_records = self.get_eeprom_earfcn_list()
                    self.prepare_eeprom_record(test_config.band, test_config.dl_freq)
                    self.enb.end_telnet_session()
                    
                    # do calibration
                    print "start calibration..."
                    self.calibration_all()
                    print ""
                    
                    if (self.report_hndl != None): 
                        self.report_hndl.flush()
                    
                im_calibration.is_dsp_running = False
                im_calibration.is_test_all = False

            elif (self.mod == '2'):
                
                self.find_eeprom_record();
                om = cal_rx_rssi.CalRxRssi(self.report_hndl, self.curr_rec_num)
                om.enb.start_telnet_session()
                om.run()
                time.sleep(2)
                
                om.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                
                om.enb.end_telnet_session()
                print "End of RSSI calibration"
                #del om
                
            elif (self.mod == '3'):

                self.find_eeprom_record();
                if (self.cfg.test_set == 'agilent'):

                    if (self.cfg.board_typ == "refkit1"):
                        om = cal_ref_clk_dac.CalRefClk(self.report_hndl, self.curr_rec_num)
                    elif (self.cfg.board_typ == "zen_ad"):
                        if (self.cfg.tcxo_ctrl == "pwm"):
                            om = cal_ref_clk_pwm.CalRefClk(self.report_hndl, self.curr_rec_num)
                        elif (self.cfg.tcxo_ctrl == "dax"):
                            om = cal_ref_clk_dax.CalRefClkDax(self.report_hndl, self.curr_rec_num)
                    else:
                        print "board not supported"
                        continue
                    
                elif (self.cfg.test_set == 'anritsu'):
                    om = cal_freq_err_pwm.CalFreqErr(self.report_hndl, self.curr_rec_num)
                elif (self.cfg.test_set == 'rs'):
                    om = cal_freq_err_pwm.CalFreqErr(self.report_hndl, self.curr_rec_num)
                    
                om.enb.start_telnet_session()
                om.run()
                """
                om.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                om.enb.end_telnet_session()
                print "End of Reference Clock calibration"
                #del om
                """
            elif (self.mod == '4'):
                
                self.find_eeprom_record();
                if (self.cfg.test_set == 'agilent'):
                    om = cal_iq_offset.CalIQOffset(self.report_hndl, self.curr_rec_num)
                elif (self.cfg.test_set == 'anritsu'):
                    om = cal_iq_offset_ms8870a.CalIQOffsetMT8870A(self.report_hndl, self.curr_rec_num)
                elif (self.cfg.test_set == 'rs'):
                    om = cal_iq_offset_cmw500.CalIQOffsetCMW500(self.report_hndl, self.curr_rec_num)
                
                om.enb.start_telnet_session()
                om.run()
                
                om.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                om.enb.end_telnet_session()
                print "End of IQ Offset calibration"
                #del om
                
            elif (self.mod == '5'):
                
                self.find_eeprom_record();
                om = cal_tx_pwr.CalTxPwr(self.report_hndl, self.curr_rec_num)
                om.enb.start_telnet_session()
                om.run()
                
                om.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                om.enb.end_telnet_session()
                print "End of TX Power calibration"
                #del om
                
            elif (self.mod == '6'):
                
                self.find_eeprom_record();
                om = test_tx_evm.TestTxEvm(self.report_hndl, self.curr_rec_num)
                om.enb.start_telnet_session()
                om.run()
                
                om.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                om.enb.end_telnet_session()
                print "End of TX power optimization"
                #del om
                
            elif (self.mod == '7'):
                
                om = cal_tx_pwr.CalTxPwr(self.report_hndl, self.curr_rec_num)
                om.enb.start_telnet_session()
                om.tx_test()
                
                om.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                
                time.sleep(2)
                om.enb.end_telnet_session()
                
                print "End of TX Power Test"
                #del om
                
            elif (self.mod == '8'):
                
                om = test_tx_cw.TestTxCw(self.report_hndl)
                om.enb.start_telnet_session()
                om.run()
                
                om.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                om.enb.end_telnet_session()
                print "End of TX CW Test"
                #del om
                
            elif (self.mod == '9'):
                
                self.start_time = time.time()
                om = test_rx_sensitivity.RxSens(self.report_hndl)
                
                om.enb.start_telnet_session()
                om.run()
                #om.run_limit_test()
                
                self.elapsed_time = float(time.time() - self.start_time)
                print "End of RX Sensitivity Test"
                print "Elapsed time = %.2f [sec]" % self.elapsed_time
                print ""
                
                om.enb.enb_reboot_in_dsp_run2()
                #print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                om.enb.end_telnet_session()
                #del om
                
            elif (self.mod == '19'):
                
                self.start_time = time.time()
                om = test_rx_sensitivity.RxSens(self.report_hndl)
                
                om.enb.start_telnet_session()
                om.run_continuous()
                
                self.elapsed_time = float(time.time() - self.start_time)
                print "End of RX Continues Test"
                print "Elapsed time = %.2f [sec]" % self.elapsed_time
                print ""
                
                om.enb.enb_reboot_in_dsp_run2()
                #print "rebooting enodeB..."
                im_calibration.is_dsp_running = False
                time.sleep(2)
                om.enb.end_telnet_session()
                #del om
                
            elif (self.mod == '10'):
                
                self.flatness_dl_freq = 0
                self.flatness_dl_freq = 0
                flatness_bw = [5, 10, 20]; # wanted testing bandwidth
                self.start_time = time.time()
                mb = eutra_bands.BandFactory.newBand(test_config.band)
                
                for bw in flatness_bw:
                    
                    for ft in range(0, 3):
                        
                        if (self.report_hndl != None):
                            print '\nBandwidth ' + str(bw) + ' MHz, test ' + str(ft+1) + '\n'
                            self.report_hndl.write('\n*** Bandwidth ' + str(bw) + 
                                                   ' MHz, test ' + str(ft+1) + ' ***\n\n')
                        
                        
                        #TODO: duplicate code in this section
                        om = test_flatness.TestFlatness(self.report_hndl, bw,
                                                        self.flatness_dl_freq)
                            
                        for cnt in range(self.telnet_retry):
                            if (om.enb.start_telnet_session() == -1):
                                time.sleep(10)
                            else:
                                break
                            if (cnt == (self.telnet_retry - 1)):
                                print "telnet connection no ready"
                                sys.exit()
                        time.sleep(15) # time from connect to login
                        om.enb.enb_login()
                        #End
                        
                        # Write uboot variables
                        if (ft == 0):
                            self.flatness_dl_freq = mb.dl_freq()[ft] + bw/2
                            self.flatness_ul_freq = mb.ul_freq()[ft] + bw/2
                        elif (ft == 2):
                            self.flatness_dl_freq = mb.dl_freq()[ft] - bw/2
                            self.flatness_ul_freq = mb.ul_freq()[ft] - bw/2
                        else:
                            self.flatness_dl_freq = mb.dl_freq()[ft]
                            self.flatness_ul_freq = mb.ul_freq()[ft]
                        
                        print "\nwrite current testing variable...\n"
                        om.enb.wr_ubootenv_bw(bw)
                        om.enb.wr_ubootenv_freq(self.flatness_dl_freq, \
                                                self.flatness_ul_freq)
                        
                        """
                        om.enb.tn_write(im_calibration.pp_base, "fsetenv bw " \
                                        + str(bw), 3)
                        om.enb.tn_write(im_calibration.pp_base, "fsetenv txfreq " \
                                        + str(self.flatness_dl_freq), 3)
                        om.enb.tn_write(im_calibration.pp_base, "fsetenv rxfreq " \
                                        + str(self.flatness_ul_freq), 3)
                        """
                        
                        # System reboot
                        time.sleep(2) # wait for writing uboot
                        om.enb.enb_reboot_in_dsp_run()
                        print "rebooting enodeB..."
                        #im_calibration.is_dsp_running = False
                        time.sleep(2)
                        om.enb.end_telnet_session()
                        del om
                        
                        om = test_flatness.TestFlatness(self.report_hndl, bw,
                                                        self.flatness_dl_freq)
                            
                        for cnt in range(self.telnet_retry):
                            if (om.enb.start_telnet_session() == -1):
                                time.sleep(10)
                            else:
                                break
                            if (cnt == (self.telnet_retry - 1)):
                                print "telnet connection no ready"
                                sys.exit()
                        time.sleep(20) # time from connet to login
                        om.run()
                        
                        om.enb.enb_reboot_in_dsp_run()
                        print "rebooting enodeB..."
                        #im_calibration.is_dsp_running = False
                        time.sleep(2)
                        om.enb.end_telnet_session()
                        del om
                        
                self.elapsed_time = float(time.time() - self.start_time)
                print "End of Transmit Flatness Test"
                print "Elapsed time = %.2f [sec]" % self.elapsed_time
                print ""
                
            elif (self.mod == '11'):
                
                if os.name == 'nt':
                    print "OS = Windows"
                elif os.name == 'posix':
                    print "OS = Linux"
                else:
                    raise "Sorry no implementation for your platform (%s) available." % sys.platform
                
                common.hit_continue("please close serial comm program (ie. minicom)")
                tt = bb_testterm.TestTerm(os.name)
                tt.run()
                del tt
                print "End of baseboard Test"
                
            elif (self.mod == '12'):
                
                self.enb.start_telnet_session()
                self.enb.enb_login()
                self.enb.enb_cd_usr_bin()
                #print "load RF driver"
                #self.enb.enb_load_rf_drv()

                self.enb.enb_eeprom_to_uboot()
                
                self.enb.enb_reboot_in_dsp_run()
                print "rebooting enodeB..."
                time.sleep(2)
                self.enb.end_telnet_session()
                
            elif (self.mod == '13'):
                # add CFR parameters here
                self.ch_gain_val = ['0x1', '0x0']
                self.rf_gain_val = ['0x20002000', '0x00220033']
                self.rf_shift_sat_val = ['0x00440033', '0x20002000', '0x1D001D00']
                
                if (self.report_hndl != None):
                    print '\nCFR Test'
                    self.report_hndl.write('\n*** CFR Test ***\n')
                    self.report_hndl.write('\n*** Bandwidth ' + \
                            str(self.cfg.cal_bandwidth) + ' MHz+ ***\n\n')
                else:
                    print 'file open error'
                    sys.exit()
                
                self.start_time = time.time()
                
                for cg in self.ch_gain_val:
                    for rg in self.rf_gain_val:
                        for rs in self.rf_shift_sat_val:
                            
                            om = test_tx_cfr.TestTxCfr(self.report_hndl, cg, rg, rs)
                            for cnt in range(self.telnet_retry):
                                if (om.enb.start_telnet_session() == -1):
                                    time.sleep(10)
                                else:
                                    break
                                if (cnt == (self.telnet_retry - 1)):
                                    print "telnet connection no ready"
                                    sys.exit()
                            time.sleep(15) # time from connect to login
                            #om.enb.enb_login()
                            om.run()
                            
                            # System reboot
                            time.sleep(2) # wait for writing uboot
                            om.enb.enb_reboot_in_dsp_run()
                            print "rebooting enodeB..."
                            #im_calibration.is_dsp_running = False
                            time.sleep(2)
                            om.enb.end_telnet_session()
                            del om
                
                self.elapsed_time = float(time.time() - self.start_time)
                print "End of Transmit CFR Test"
                print "Elapsed time = %.2f [sec]" % self.elapsed_time
                print ""
                
            elif (self.mod == '21'):
                
                print 'read baseboard data'
                om = bb_eeprom.BBEepromAccess()
                om.enb.start_telnet_session()
                om.start_enodeb()
                om.read_bb_eeprom()
                om.enb.end_telnet_session()
                
            elif (self.mod == '22'):
                
                print 'write baseboard data'
                om = bb_eeprom.BBEepromAccess()
                om.enb.start_telnet_session()
                om.start_enodeb()
                om.write_bb_eeprom()
                om.enb.end_telnet_session()
                
            elif (self.mod == '23'):
                
                om = bb_eeprom.BBEepromAccess()
                om.enb.start_telnet_session()
                om.start_enodeb()
                om.erase_bb_eeprom()
                om.enb.end_telnet_session()
                
            elif (self.mod == 'q') or (self.mod == 'Q'):
                break
            
            else:
                print "unknown option"
                continue
        
        if (self.report_hndl != None): 
            self.report_hndl.flush()
            self.close_test_report()
            
        #ic.end_sys()
        
    def do_test(self, o_item):
        
        o_item.enb.start_telnet_session()
        o_item.run()
        
        o_item.enb.enb_reboot_in_dsp_run()
        time.sleep(2)
        o_item.enb.end_telnet_session()
        print "rebooting enodeB..."
        time.sleep(self.reboot_wait)
                
def main():

    mn = rfCardCal()
    mn.run()
    print "\nEnd of RF Card Calibration"
    sys.exit()

if (__name__ == "__main__"):
    main()
