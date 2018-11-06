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
from cmd import PROMPT

class CalRefClk(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.cur_freq = 0.0
        self.dif_freq = 0
        self.pwm_cur = 0x6F50
        self.pwm_val = 0x6F50
        self.pwm_lo_val = 0x9200
        self.limit_cal = 10 #3 #frequency offset limit
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
        type(self).enb.enb_cd_usr_bin()
        
        # default is PWM control in booting
        if (type(self).cfg.tcxo_ctrl == "dax"):
            print("load ext dac clock control")
            type(self).enb.enb_load_dax()   # do ext DAC control
    
        if test_config.band > 32:
            type(self).enb.enb_set_zen_tdd_tx()
        """
        if im_calibration.is_dsp_running:
            type(self).enb.enb_cd_tmpfs()
            type(self).enb.enb_stop_transmit()
        """
    def do_refclk_cal(self):
        
        # set initial local frequency
        type(self).enb.enb_pwm_ctrl_call_cmd('l ' + str(self.pwm_lo_val) + 
                                             ' h ' + str(self.pwm_val) + ' e q')

        type(self).mxa.send_msg_to_server(':CALC:MARK1:CPS ON')    # mark peak
        sleep(3)    # wait for frequency stable
        
        for rn in range(80):
            print("\ncalibration round: " + str(rn+1))
            self.check_curr_freq()
            
            self.dif_freq = int(round(self.ref_freq - self.cur_freq))
            
            if rn > 30:
                self.limit_cal = 20 #20;
            
            if (abs(self.dif_freq) < 60) and (not self.narrow_span):
                type(self).mxa.send_msg_to_server(':FREQ:SPAN 1000 Hz')
                #type(self).mxa.send_msg_to_server(':BAND 1 Hz')
                self.narrow_span = True
                sleep(1) #(3)
                continue
            elif (abs(self.dif_freq) >= self.limit_cal):
                self.calc_pwm_value()
            else:
                #print "freq_diff=" + str(abs(self.dif_freq)) + ", limit_cal=" + str(self.limit_cal)
                self.got_ref = True
                print("Got target reference frequency,")
                print("    PWM_high value= " + str(self.pwm_val))
                print("    PWM_low value= " + str(self.pwm_lo_val))
                if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                    self.rpt.write('\nReference Clock Calibration:\n')
                    self.rpt.write('Frequency = ' + str(self.ref_freq) + '\n')
                    self.rpt.write('PWM_high value = ' + str(self.pwm_val) + '\n')
                    self.rpt.write('PWM_low value = ' + str(self.pwm_lo_val) + '\n')
                break
            
            type(self).enb.enb_pwm_ctrl_call_cmd('h ' + str(self.pwm_val) + ' q')
            sleep(1) #(2)
        
        if (not self.got_ref):
            print("Reference clock calibration failed")
        else:
            
            # pwmreg0 for high pwm, pwmreg1 for low pwm
            if test_config.wr_var_to_uboot == True:
                #print("fsetenv pwm_offset " + str(self.pwm_val))
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv pwmreglow " \
                                        + str(self.pwm_lo_val), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv pwmreghigh " \
                                        + str(self.pwm_val), 3)
            
            if (type(self).cfg.en_eeprom_write == True):
                self.enb.editUbootenv('PWMREGHIGH', str(self.pwm_val))
                self.enb.editUbootenv('PWMREGLOW', str(self.pwm_lo_val))
                
                if (im_calibration.is_test_all == True):
                    self.write_pwmclk_to_eeprom()
                else:
                    se = raw_input("Write result to EEPROM?(y/n):")
                    if (se == 'y') or (se == 'Y'):
                        
                        print("fsetenv pwm_offset " + str(self.pwm_val))
                        type(self).enb.tn_write(im_calibration.pp_base, "fsetenv pwmreglow " \
                                                + str(self.pwm_lo_val), 3)
                        type(self).enb.tn_write(im_calibration.pp_base, "fsetenv pwmreghigh " \
                                                + str(self.pwm_val), 3)
                        
                        self.write_pwmclk_to_eeprom()
            
    def write_pwmclk_to_eeprom(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        print "edit EEPROM record"
        type(self).enb.enb_eeprom_edit_record('wrh', self.rn, str(self.pwm_val))
        type(self).enb.enb_eeprom_edit_record('wrl', self.rn, str(self.pwm_lo_val))
        sleep(0.5)    # wait EEPROM data wrote
        
    def check_curr_freq(self):
        
        type(self).mxa.send_msg_to_server(':CALC:MARK1:X?')
        in_msg = type(self).mxa.recv_msg_frm_server()
        self.cur_freq = float(in_msg)
        print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)        
        
    def calc_pwm_value(self):

        self.pwm_val = self.pwm_cur + self.dif_freq * self.dif_factor
        
        print("pwm_val=" + str(self.pwm_cur) + " diff=" + str(self.dif_freq) 
              + " new_val=" + str(self.pwm_val))
        self.pwm_cur = self.pwm_val  
        
    def run(self):
        
        self.mxa_setup()
        self.start_enodeb()
        self.do_refclk_cal()
        
