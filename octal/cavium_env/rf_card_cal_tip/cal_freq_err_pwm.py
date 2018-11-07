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

class CalFreqErr(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.dif_freq = 0
        self.pwm_ini = 0x5000
        self.pwm_cur = 0x5000
        self.pwm_val = 0x5000
        self.pwm_lo_val = 0x9200
        self.limit_cal = 10
        self.dif_factor = 1
        self.got_ref = False
        self.narrow_span = False
        self.ref_freq = test_config.dl_freq * 1000000
        self.input_power = 0    # input power in dBm
        self.signal_bw = 10     # input signal bandwidth
        
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
        
    def mt8870a_setup(self, freq_center):
        
        print 'Start VSA...'
        type(self).mt8870a.send_msg_to_server(':BATC:BAND:FREQ:CENT ' + str(freq_center) + 'MHZ')
        type(self).mt8870a.send_msg_to_server(':BATC:BAND:POW:RANG:ILEV ' + str(self.input_power))
        type(self).mt8870a.send_msg_to_server(':BATC:CC:RAD:CBAN ' + str(self.signal_bw))
        type(self).mt8870a.send_msg_to_server(':BATC:CC:RAD:TMOD TM1_1')
        
        type(self).mt8870a.send_msg_to_server(':TRIG:STAT OFF')
        type(self).mt8870a.send_msg_to_server(':TRIG:SOUR IMM')
        
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:STAR 0')
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:LENG 1')
        
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:UWEM:STAR 0')
        type(self).mt8870a.send_msg_to_server(':BATC:CAPT:TIME:UWEM:LENG 140')
        type(self).mt8870a.send_msg_to_server(':BATC:CC:PDCC:SYMB:NUMB 2')
        
        type(self).mt8870a.send_msg_to_server(':BATC:EVM ON')
        type(self).mt8870a.send_msg_to_server('*WAI')

    def cmw500_setup(self, freq_center):
        
        print 'Start VSA...'
        #if test_config.band > 32:
        #    type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:DMOD TDD")
        #else:
        #    type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:DMOD FDD")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:DMOD FDD")    
        type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFAC, RX1")
            
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:FREQ " + str(freq_center) + " MHz")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:ENP 35")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:UMAR 0")
        
        if (self.signal_bw == 5):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B050")
        elif (self.signal_bw == 10):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B100")
        elif (self.signal_bw == 15):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B150")
        elif (self.signal_bw == 20):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B200")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:RES:ALL ON,OFF,OFF,OFF,OFF,OFF,ON,OFF,OFF")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:SCO:MOD 10")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:MOEX ON")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:REP SING")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:ETES ETM11")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:PLC 1")
        type(self).cmw500.send_msg_to_server("TRIG:LTE:MEAS:ENB:MEV:SOUR 'Free Run (Fast Sync)'")

        
     
        
    def start_enodeb(self):
        
        type(self).enb.enb_login()
        type(self).enb.enb_cd_usr_bin()
        
        # default is PWM control in booting
        if (type(self).cfg.tcxo_ctrl == "dax"):
            print("load ext dac clock control")
            type(self).enb.enb_load_dax()   # do ext DAC control
    
        if test_config.band > 32:
            type(self).enb.enb_set_zen_tdd_tx()
            
        type(self).enb.enb_cd_tmpfs()
        type(self).enb.enb_run_dsp_app_dl()
        
    def do_refclk_cal(self):
        
        type(self).enb.enb_pwm_ctrl_call_cmd('l ' + str(self.pwm_lo_val) +
                                             ' h ' + str(self.pwm_ini) + ' e q')
        
        for rn in range(50):
            print("\ncalibration round: " + str(rn+1))
            self.check_freq_err()
            
            if rn > 30:
                self.limit_cal = 50;
            
            if (abs(self.dif_freq) >= self.limit_cal):
                self.calc_pwm_value()
            else:
                self.got_ref = True
                print("\nGot target reference frequency,")
                print("    PWM_high value= " + str(self.pwm_val))
                print("    PWM_low value= " + str(self.pwm_lo_val))
                if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                    self.rpt.write('\nReference Clock Calibration:\n')
                    self.rpt.write('Frequency = ' + str(self.ref_freq) + '\n')
                    self.rpt.write('PWM_high value = ' + str(self.pwm_val) + '\n')
                    self.rpt.write('PWM_low value = ' + str(self.pwm_lo_val) + '\n')
                break
            
            type(self).enb.enb_pwm_ctrl_call_cmd('h ' + str(self.pwm_val) + ' q')
            sleep(3)
        
        if (not self.got_ref):
            print("Reference clock calibration failed")
        else:
            
            # pwmreg0 for high pwm, pwmreg1 for low pwm
            if test_config.wr_var_to_uboot == True:
                print("fsetenv pwm_offset " + str(self.pwm_val))
                type(self).enb.tn_write(im_calibration.pp_base, \
                                "fsetenv pwmreglow " + str(self.pwm_lo_val), 3)
                type(self).enb.tn_write(im_calibration.pp_base, \
                                "fsetenv pwmreghigh " + str(self.pwm_val), 3)
            
            if (type(self).cfg.en_eeprom_write == True):
                if (im_calibration.is_test_all == True):
                    self.write_pwmclk_to_eeprom()
                else:
                    se = raw_input("Write result to EEPROM?(y/n):")
                    if (se == 'y') or (se == 'Y'):
                        self.write_pwmclk_to_eeprom()
                        
                    se = raw_input("Write result to uboot?(y/n):")
                    if (se == 'y') or (se == 'Y'):
                        print("fsetenv pwm_offset " + str(self.pwm_val))
                        type(self).enb.tn_write(im_calibration.pp_base, \
                                                "fsetenv pwmreglow " \
                                                + str(self.pwm_lo_val), 3)
                        type(self).enb.tn_write(im_calibration.pp_base, \
                                                "fsetenv pwmreghigh " \
                                                + str(self.pwm_val), 3)
            
    def write_pwmclk_to_eeprom(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        print "edit EEPROM record"
        type(self).enb.enb_eeprom_edit_record('wrh', self.rn, str(self.pwm_val))
        type(self).enb.enb_eeprom_edit_record('wrl', self.rn, str(self.pwm_lo_val))
        sleep(0.5)    # wait EEPROM data wrote
     
    def check_freq_err(self):
        
        if (type(self).cfg.test_set == 'anritsu'):
            
            type(self).mt8870a.send_msg_to_server(':INIT:MODE:SING')
            type(self).mt8870a.send_msg_to_server('*WAI')
            type(self).mt8870a.send_msg_to_server(':FETC:BATC1?')
            type(self).mt8870a.send_msg_to_server('*WAI')
            #sleep(3)
            in_msg = type(self).mt8870a.recv_msg_frm_server()
            #print in_msg
            line = in_msg.split(',')
            self.dif_freq = int(float(line[1]))
            print "frequency error = " + str(self.dif_freq) + " Hz"
            
        elif (type(self).cfg.test_set == 'rs'):
        	  
        	  type(self).cmw500.send_msg_to_server('INIT:LTE:MEAS:ENB:MEV')
        	  type(self).cmw500.send_msg_to_server('*WAI')
        	  type(self).cmw500.send_msg_to_server('FETC:LTE:MEAS:ENB:MEV:MOD:AVER?')
        	  in_msg = type(self).cmw500.recv_msg_frm_server()
        	  line = in_msg.split(',')
        	  if (int(line[0]) != 0):
        	      print in_msg
        	  self.dif_freq = int(float(line[15]))
        	  print "frequency error = " + str(self.dif_freq) + " Hz"
    
    def calc_pwm_value(self):
        
        if (abs(self.dif_freq) > 10000):
            self.pwm_val = self.pwm_cur + 1000
            print("pwm_val=" + str(self.pwm_cur) + " diff=" + str(self.dif_freq) 
                  + " new_val=" + str(self.pwm_val))
        else:
            self.pwm_val = self.pwm_cur - self.dif_freq * self.dif_factor
            print("pwm_val=" + str(self.pwm_cur) + " diff=" + str(self.dif_freq) 
                  + " new_val=" + str(self.pwm_val))
            
        self.pwm_cur = self.pwm_val  
        
    def run(self):
        
        self.start_enodeb()
        sleep(3)    # wait spectrum comes out
        
        if (type(self).cfg.test_set == 'agilent'):
            self.mxa_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(test_config.dl_freq)
        elif (type(self).cfg.test_set == 'rs'):
        	  self.cmw500_setup(test_config.dl_freq)
            
        self.do_refclk_cal()
        