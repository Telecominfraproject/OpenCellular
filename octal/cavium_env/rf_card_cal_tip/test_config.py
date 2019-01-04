#!/usr/bin/env python

"""
description: test configuration
"""

import os
import sys

# band, frequency setting:
# keep zero if select band later and use middle channel to test
# fill target band and frequency for custom setting
band = 0                            # DUT band
dl_freq = 0                         # current downlink frequency
ul_freq = 0                         # current uplink frequency
wr_var_to_uboot = True             # T: write variable to uboot; F: don't write

class EnbConfig():
    
    def __init__(self):

        self.num_cal_channel = 0    # number of calibration channels
        self.dl_freq_arr = []       # list of downlink frequencies
        self.ul_freq_arr = []       # list of uplink frequencies
        self.cal_freq_arr = []      # list of calibration freq pair
        
        self.cfg_file = 'test_config.txt'
        
        self.board_typ = 'zen_ad'   # board type [refkit1, zen_ad, zen_ak]
        self.cal_bandwidth = 20     # calibration bandwidth in mhz [10, 20]
        self.tcxo_ctrl = 'pwm'      # tcxo control type [pwm, dac, dax]
        
        self.attn1 = 13             # port 1 attenuation
        self.attn2 = 13             # port 2 attenuation
        self.gain1 = 35             # port 1 gain [dB]
        self.gain2 = 35             # port 2 gain [dB]
        
        self.login_pwd = ''         # root login password
        self.enb_ipaddr = '10.18.104.61' #'192.168.166.61'      # enodeB IP address
        self.enb_tn_port = 23                   # enodeB telnet port
        self.tftp_server_ip = '10.18.104.240'   #'192.168.166.202' # tftp server IP address
        self.rssi_cable_loss = 5                # cable loss for RSSI test
        self.txpwr_cable_loss = 5               # cable loss for TX test
        self.manual_switch_instr = True         # enable/disable wait for switch cable
        
        self.test_set='rs'          # test set type [agilent, anritsu, rs]
        
        # agilent test set
        self.exg_ipaddr = '192.168.166.201'     # EXG IP address
        self.exg_tcp_port = 5025                # EXG TCP port
        self.mxa_ipaddr = '192.168.166.203'      # MXA IP address
        self.mxa_tcp_port = 5025                # MXA TCP port
        
        # anritsu test set
        self.mt8870a_ipaddr = '192.168.166.201'
        self.mt8870a_tcp_port = 56001
        
        # r&s test set
        self.cmw500_ipaddr = '192.168.166.69'
        self.cmw500_tcp_port = 5025
        
        # test files
        self.rf_driver = 'cn_rfdriver'
        self.rf_drv_init = 'rf_init.txt'
        self.dsp_app_dl = 'pltD-dl'
        self.dl_etm_test_vector = 'CAL_ETM_TV_20Mhz.tgz'
        self.exg_waveform = '"ESG_TC0301_10M_LOW.DAT"'

        # test criteria
        self.max_atten = 25         # make the lowest TX power to start TX power test; 5 ~ 45
        self.cr_txpwr_min = 11      # minimum TX output power limit
        self.cr_txpwr_max = 22      # maximum TX output power limit
        self.cr_txevm_max = 3.5     # maximum TX EVM limit
        
        # system variables
        self.en_eeprom_write = True # disable when refkit1
        self.eeprom_record_ver = 1  # EEPROM record version
        self.test_report = True     # T: enable test report; F: disable
        self.instr_disp = True      # T: enable instrument screen display
        
        self.eeprom_board_type = 1
        self.eeprom_rfic_type = 1
        self.eeprom_tcxo_ctrl = 1   # tcxo control type
        
        # RX sensitivity test
        self.do_sens_test_in_opt1 = True        # do test in option 1 calibration
        self.my_udp_ipaddr = '10.102.81.151'    # UDP IP address for sensitivity test
        self.my_udp_port = 9991                 # UDP port for sensitivity test
        self.rx_test_vector = 'TV_Low_120627.tgz'   # for old DSP
        self.rx_patch_vector = 'tc0301_low.tgz'     # for old DSP
        self.tcid = '901'                       # RX test case ID
        self.rx_gain = 60                       # RX gain for sensitivity test
        self.bler_limit = 5                     # percentage of BLER pass criteria
        self.sens_pass_limit = -96              # sensitivity pass limit
        
        # Baseboard test variables
        self.bb_port = 'COM11'  #'/dev/ttyUSB0'
        self.bb_baudrate = 115200
        self.bb_pingip = '10.18.104.240'
        
        if self.check_cfg_file() < 0: sys.exit()
        # only running read_cal_channel once during the call from main, not every init of EnbConfig
        # if self.read_cal_channel() < 0: sys.exit()
        
        self.read_config()
        self.select_rf_drv_init()
        self.select_dl_etm_test_vector()
        self.select_ul_exg_waveform()
        
    def check_cfg_file(self):
        
        cfgfile = open(self.cfg_file, "r")
        
        if (os.path.isfile(self.cfg_file) == False):
            print self.cfg_file + " file doesn't exist"
            return -1
        else:
            self.cfgln = cfgfile.readlines()
            cfgfile.close()
            return 0
        
    def read_cal_channel(self):

        se = raw_input("Which band are you running?(3/5/28):")
        if (se == "3"):
            band_suffix = "_3"
        elif (se == "5"):
            band_suffix = "_5"
        elif (se == "28"):
            band_suffix = "_28"
        else:
            print se + " is not a supported band"
            return -1

        band_name = "band" + band_suffix
        dl_freq = "dl_freq" + band_suffix
        ul_freq = "ul_freq" + band_suffix

        for cl in self.cfgln:
            par, val = self.read_line(cl)

            if (par == band_name):
                global band
                band = int(val)
            elif (par == "num_cal_channel"):
                self.num_cal_channel = int(val)
            elif (par == dl_freq):
                self.dl_freq_arr.append(float(val))
            elif (par == ul_freq):
                self.ul_freq_arr.append(float(val))
                        
        if (len(self.dl_freq_arr) != len(self.ul_freq_arr)):
            print "number of downlink and uplink frequencies mismatch"
            return -1
        
        if (len(self.dl_freq_arr) != self.num_cal_channel):
            print "num_cal_channel value mismatch with DL/UL frequencies"
            return -1 
        
        for i in range(self.num_cal_channel):
            self.cal_freq_arr.append([self.dl_freq_arr[i], self.ul_freq_arr[i]])
            #print "dl_freq_arr " + str(self.dl_freq_arr[i]) + " ul_freq_arr " + str(self.ul_freq_arr[i])
         
        return 0
                        
    def get_cal_freq_arr(self):
        
        return self.cal_freq_arr
        
    def select_dl_etm_test_vector(self):
        
        if (self.cal_bandwidth == 5):
            self.dl_etm_test_vector = 'CAL_ETM_TV_5Mhz.tgz'
        elif (self.cal_bandwidth == 10):
            self.dl_etm_test_vector = 'CAL_ETM_TV_10Mhz.tgz'
        elif (self.cal_bandwidth == 15):
            self.dl_etm_test_vector = 'CAL_ETM_TV_15Mhz.tgz'
        elif (self.cal_bandwidth == 20):
            self.dl_etm_test_vector = 'CAL_ETM_TV_20Mhz.tgz'
        else:
            self.dl_etm_test_vector = 'CAL_ETM_TV_20Mhz.tgz'
                
    def select_ul_exg_waveform(self):
        
        if (self.cal_bandwidth == 5):
            if (self.tcid == '901'):
                self.exg_waveform = '"5MHz_FRCA13_RO_0"'
            elif (self.tcid == '307'):
                self.exg_waveform = '"5mhz__pusch_15rb_frca3_4_tc0307.wfm"'
            elif (self.tcid == '308'):
                self.exg_waveform = '"5mhz__pusch_25rb_frca4_5_tc0308.wfm"'
            elif (self.tcid == '309'):
                self.exg_waveform = '"5mhz__pusch_25rb_frca5_4_tc0309.wfm"'
            else:
                self.exg_waveform = '"5mhz__pusch_15rb_frca1_2_tc0306.wfm"'
        elif (self.cal_bandwidth == 10):
            if (self.tcid == '901'):
                #self.exg_waveform = '"10mhz__pusch_25rb_frca1_3_tc0901.wfm"'
                self.exg_waveform = '"10MHz_FRCA13_RO_0"'
            elif (self.tcid == '902'):
                self.exg_waveform = '"10mhz__pusch_50rb_frca3_5_tc0902.wfm"'
            elif (self.tcid == '903'):
                self.exg_waveform = '"10mhz__pusch_50rb_frca4_6_tc0903.wfm"'
            elif (self.tcid == '904'):
                self.exg_waveform = '"10mhz__pusch_50rb_frca5_5_tc0904.wfm"'
            else:
                self.exg_waveform = '"10mhz__pusch_25rb_frca1_3_tc0901.wfm"'
        elif (self.cal_bandwidth == 15):
            if (self.tcid == '901'):
                self.exg_waveform = '"15MHZ_FRCA13_RO_0"'
        elif (self.cal_bandwidth == 20):
            if (self.tcid == '901'):
                #self.exg_waveform = '"20mhz__pusch_25rb_frca1_3_tc0901.wfm"'
                self.exg_waveform = '"20MHz_FRCA13_RO_0"'
            elif (self.tcid == '902'):
                self.exg_waveform = '"20mhz__pusch_100rb_frca3_7_tc0902.wfm"'
            elif (self.tcid == '903'):
                self.exg_waveform = '"20mhz__pusch_100rb_frca4_8_tc0903.wfm"'
            elif (self.tcid == '904'):
                self.exg_waveform = '"20mhz__pusch_100rb_frca5_7_tc0904.wfm"'
            else:
                self.exg_waveform = '"20mhz__pusch_25rb_frca1_3_tc0901.wfm"'
        else:
            self.exg_waveform = '"10mhz__pusch_25rb_frca1_3_tc0901.wfm"'

    def select_rf_drv_init(self):
        
        if (self.cal_bandwidth == 5):
            self.rf_drv_init = 'ad9362_init_zen5Mhz.txt'
        elif (self.cal_bandwidth == 10):
            self.rf_drv_init = 'ad9362_init_zen10Mhz.txt'
        elif (self.cal_bandwidth == 15):
            self.rf_drv_init = 'ad9362_init_zen15Mhz.txt'
        elif (self.cal_bandwidth == 20):
            self.rf_drv_init = 'ad9362_init_zen20Mhz.txt'
        else:
            self.rf_drv_init = 'ad9362_init_zen20Mhz.txt'
            
                
    def read_config(self):
        
        cfgfile = open(self.cfg_file, "r")
        cfgln = cfgfile.readlines()
        cfgfile.close()
        
        for cl in cfgln:
            par, val = self.read_line(cl)
            if (par == "board_typ"):
                self.board_typ = val
            elif (par == "cal_bandwidth"):
                self.cal_bandwidth = int(val)
            elif (par == "tcxo_ctrl"):
                self.tcxo_ctrl = val
            elif (par == "test_set"):
                self.test_set = val
            elif (par == "attn1"):
                self.attn1 = val
            elif (par == "attn2"):
                self.attn2 = val
            elif (par == "gain1"):
                self.gain1 = val
            elif (par == "gain2"):
                self.gain2 = val
            elif (par == "login_pwd"):
                self.login_pwd = val
            elif (par == "enb_ipaddr"):
                self.enb_ipaddr = val
            elif (par == "enb_tn_port"):
                self.enb_tn_port = int(val)
            elif (par == "tftp_server_ip"):
                self.tftp_server_ip = val
            elif (par == "exg_ipaddr"):
                self.exg_ipaddr = val
            elif (par == "exg_tcp_port"):
                self.exg_tcp_port = int(val)
            elif (par == "mxa_ipaddr"):
                self.mxa_ipaddr = val
            elif (par == "mxa_tcp_port"):
                self.mxa_tcp_port = int(val)
            elif (par == "mt8870a_ipaddr"):
                self.mt8870a_ipaddr = val
            elif (par == "mt8870a_tcp_port"):
                self.mt8870a_tcp_port = int(val) 
            elif (par == "cmw500_ipaddr"):
                self.cmw500_ipaddr = val
            elif (par == "cmw500_tcp_port"):
                self.cmw500_tcp_port = int(val) 
            elif (par == "rssi_cable_loss"):
                self.rssi_cable_loss = float(val)
            elif (par == "txpwr_cable_loss"):
                self.txpwr_cable_loss = float(val)
            elif (par == "manual_switch_instr"):
                self.manual_switch_instr = self.str2bool(val)
            elif (par == "rf_driver"):
                self.rf_driver = val
            #elif (par == "rf_drv_init"):
            #    self.rf_drv_init = val
            elif (par == "dsp_app_dl"):
                self.dsp_app_dl = val
            #elif (par == "dl_etm_test_vector"):
            #    self.dl_etm_test_vector = val
            #elif (par == "exg_waveform"):
            #    self.exg_waveform = val
            elif (par == "max_atten"):
                self.max_atten = int(val)
            elif (par == "cr_txpwr_min"):
                self.cr_txpwr_min = int(val)
            elif (par == "cr_txpwr_max"):
                self.cr_txpwr_max = int(val)
            elif (par == "cr_txevm_max"):
                self.cr_txevm_max = float(val)
            elif (par == "en_eeprom_write"):
                self.en_eeprom_write = self.str2bool(val)
            elif (par == "eeprom_record_ver"):
                self.eeprom_record_ver = val
            #elif (par == "eeprom_new_rec"):
            #    self.eeprom_new_rec = self.str2bool(val)
            elif (par == "test_report"):
                self.test_report = self.str2bool(val)
            elif (par == "instr_disp"):
                self.instr_disp = self.str2bool(val)
            elif (par == "do_sens_test_in_opt1"):
                self.do_sens_test_in_opt1 = self.str2bool(val)
            elif (par == "my_udp_ipaddr"):
                self.my_udp_ipaddr = val
            elif (par == "my_udp_port"):
                self.my_udp_port = int(val)
            #elif (par == "rx_test_vector"):
            #    self.rx_test_vector = val
            #elif (par == "rx_patch_vector"):
            #    self.rx_patch_vector = val
            elif (par == "tcid"):
                self.tcid = val
            elif (par == "rx_gain"):
                self.rx_gain = val
            elif (par == "bler_limit"):
                self.bler_limit = int(val)
            elif (par == "sens_pass_limit"):
                self.sens_pass_limit = int(val)
            elif (par == "bb_port"):
                self.bb_port = val
            elif (par == "bb_baudrate"):
                self.bb_baudrate = val
            elif (par == "bb_pingip"):
                self.bb_pingip = val
            elif (par == ""):
                continue
            else:
                pass
                #print("unknown parameter!")
            
    def read_line(self, strline):
        param = ""
        value = ""
        sn = strline
        
        if ((sn[0] != '#') and (sn[0] != ' ') and 
            (sn[0] != '\n') and (sn[0] != '\r') and
            (sn[0] != '\t')):
            result = sn.split('=')
            param = result[0]
            value = result[1]
            for i in range(len(value)):
                if ((value[i] == '\n') or (value[i] == '\r')):
                    value = value[:i]
                    break
                elif ((value[i] == " ") or (value[i] == "#") or (value[i] == '\t')):
                        value = value[:i]
                        break
            value = value.strip("\'")
            #print("read_line(): param=%s, value=%s" % (param, value))
            
        return param, value
    
    def str2bool(self, vs):
        
        return vs.lower() in ("yes", "true", "t", "1")
    
def main():

    ec = EnbConfig()
    ec.read_config()


if (__name__ == "__main__"):
    main()
            
