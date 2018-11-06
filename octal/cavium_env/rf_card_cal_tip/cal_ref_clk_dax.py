#!/usr/bin/python

"""
Setup:
1. Agilent MXA, connect cable to one of the antenna port of enodeB
2. Set TFTP server, address and port as test_config
"""

import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration

class CalRefClkDax(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.cur_freq = 0.0
        self.dif_freq = 0
        self.dax_cur = 0x4000
        self.dax_val = 0x4000
        self.dax_lo_val = 0x9200
        self.limit_cal = 3
        self.dif_factor = 1
        self.got_ref = False
        self.narrow_span = False
        self.ref_freq = test_config.dl_freq * 1000000
        
    def mxa_setup(self):
        
        type(self).mxa.send_msg_to_server('*RST')
        type(self).mxa.send_msg_to_server('*IDN?')
        in_msg = type(self).mxa.recv_msg_frm_server()
        print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':CONF:SAN:NDEF')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(self.ref_freq) + ' Hz')
        type(self).mxa.send_msg_to_server(':FREQ:SPAN 50 KHz')
        #type(self).mxa.send_msg_to_server(':BAND 100 Hz')
        type(self).mxa.send_msg_to_server(':DISP:WIND:TRAC:Y:RLEV 10 dBm')
        type(self).mxa.send_msg_to_server(':POW:ATT 20')
        
    def start_enodeb(self):
        
        type(self).enb.enb_login()
        #type(self).enb.get_macaddr()
        type(self).enb.enb_set_1pps('tx')

        type(self).enb.enb_cd_usr_bin()
        print 'enb load rf drv'
        type(self).enb.enb_load_rf_drv()
        print 'enb load rf init'
        type(self).enb.enb_load_rf_init()
        type(self).enb.enb_set_rf_drv_rf_card()
    
        if test_config.band > 32:
            type(self).enb.enb_set_zen_tdd_tx()
        
        if im_calibration.is_dsp_running:
            type(self).enb.enb_cd_tmpfs()
            type(self).enb.enb_stop_transmit()
        
    def do_refclk_cal(self):
        
        type(self).enb.enb_dax_ctrl_call_cmd('0x3000 q')

        type(self).mxa.send_msg_to_server(':CALC:MARK1:CPS ON')    # mark peak
        sleep(3)    # wait for frequency stable
        
        for rn in range(50):
            print("\ncalibration round: " + str(rn+1))
            self.check_curr_freq()
            
            self.dif_freq = int(round(self.ref_freq - self.cur_freq))
            
            if rn > 30:
                self.limit_cal = 10;
            
            if (abs(self.dif_freq) < 50) and (not self.narrow_span):
                type(self).mxa.send_msg_to_server(':FREQ:SPAN 1000 Hz')
                #type(self).mxa.send_msg_to_server(':BAND 1 Hz')
                self.narrow_span = True
                sleep(3)
                continue
            elif (abs(self.dif_freq) >= self.limit_cal):
                self.calc_dax_value()
            else:
                self.got_ref = True
                print("Got target reference frequency,")
                print("    PWM_high value= " + str(self.dax_val))
                print("    PWM_low value= " + str(self.dax_lo_val))
                if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                    self.rpt.write('\nReference Clock Calibration:\n')
                    self.rpt.write('Frequency = ' + str(self.ref_freq) + '\n')
                    self.rpt.write('PWM_high value = ' + str(self.dax_val) + '\n')
                    self.rpt.write('PWM_low value = ' + str(self.dax_lo_val) + '\n')
                break
            
            type(self).enb.enb_dax_ctrl_call_cmd(str(self.dax_val) + ' q')
            sleep(2)
        
        if (not self.got_ref):
            print("Reference clock calibration failed")
        else:
            
            # pwmreg0 for dax
            if test_config.wr_var_to_uboot == True:
                print("fsetenv dax_offset " + str(self.dax_val))
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv pwmreglow " \
                                        + str(self.dax_lo_val), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv pwmreghigh " \
                                        + str(self.dax_val), 3)
            
            if (type(self).cfg.en_eeprom_write == True):
                if (im_calibration.is_test_all == True):
                    self.enb.editUbootenv('PWMREGHIGH', str(self.dax_val))
                    self.enb.editUbootenv('PWMREGLOW', str(self.dax_lo_val))
                    self.write_daxclk_to_eeprom()
                else:
                    se = raw_input("Write result to EEPROM?(y/n):")
                    if (se == 'y') or (se == 'Y'):
                        self.enb.editUbootenv('PWMREGHIGH', str(self.dax_val))
                        self.enb.editUbootenv('PWMREGLOW', str(self.dax_lo_val))
                        self.write_daxclk_to_eeprom()
            
    def write_daxclk_to_eeprom(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        print "edit EEPROM record"
        type(self).enb.enb_eeprom_edit_record('wrh', self.rn, str(self.dax_val))
        type(self).enb.enb_eeprom_edit_record('wrl', self.rn, str(self.dax_lo_val))
        sleep(0.5)    # wait EEPROM data wrote
        
    def check_curr_freq(self):
        
        type(self).mxa.send_msg_to_server(':CALC:MARK1:X?')
        in_msg = type(self).mxa.recv_msg_frm_server()
        self.cur_freq = float(in_msg)
        print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)        
        
    def calc_dax_value(self):

        self.dax_val = self.dax_cur + self.dif_freq * self.dif_factor
        
        print("dax_val=" + str(self.dax_cur) + " diff=" + str(self.dif_freq) 
              + " new_val=" + str(self.dax_val))
        self.dax_cur = self.dax_val  
        
    def run(self):
        
        self.mxa_setup()
        self.start_enodeb()
        self.do_refclk_cal()
        
