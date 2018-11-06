#!/usr/bin/python

"""
Setup:
1. Agilent EXG, connect to primary antenna port of enodeB
2. Set TFTP server, address and port as test_config
"""

import sys
import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration
#from win32con import RES_CURSOR

class CalRxRssi(Calibration):
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
        self.slope_1 = ''
        self.offset_1 = ''
        self.slope_2 = ''
        self.offset_2 = ''
        self.init_pow = -30
        
    def exg_setup(self):
        
        type(self).exg.send_msg_to_server(':RAD:ARB ON')
        type(self).exg.send_msg_to_server(':RAD:ARB:WAV ' + type(self).cfg.exg_waveform)
        #type(self).exg.send_msg_to_server(':MMEM:DATA ' + type(self).cfg.exg_waveform)
        print("EXG waveform is " + type(self).cfg.exg_waveform)
        type(self).exg.send_msg_to_server(':FREQ ' + str(test_config.ul_freq) + ' MHz')
        type(self).exg.send_msg_to_server(':POW ' + str(self.init_pow) + ' dBm')
        type(self).exg.send_msg_to_server(':OUTP:MOD ON')
        type(self).exg.send_msg_to_server(':OUTP ON')
        
    def mt8870a_setup(self, freq_center):
        
        Waveform = "'E-TM_1-1_10M'"
        print 'VSG Setup...'
        type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:MODE NORMAL\n')
        type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:BBM ARB \n')
        type(self).mt8870a.send_msg_to_server(':SOUR:GPRF:GEN:ARB:FILE:LOAD ' + Waveform)
        type(self).mt8870a.send_msg_to_server('*WAI\n')
        type(self).mt8870a.send_msg_to_server('*WAI\n')
        type(self).mt8870a.send_msg_to_server(':SOUR:GPRF:GEN:RFS:LEV -100\n')
        print 'Loading waveform...'
        type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:ARB:WAV:PATT:SEL ' + Waveform + ',1,1\n')
        type(self).mt8870a.send_msg_to_server('*WAI\n')
        print 'Start VSG...'
        type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:RFS:FREQ ' + str(freq_center * 1000000) + '\n')
        type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:RFS:LEV ' + str(self.init_pow) + '\n')
        type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:STAT ON\n')
        
    def cmw500_setup(self, freq_center):
        
        if self.cfg.cal_bandwidth == 20:
        	  sync_delay = "0"
        	  Waveform = "'D:\Rohde-Schwarz\CMW\Data\Waveform\CaviumFRC_20MHz.wv'"
        elif self.cfg.cal_bandwidth == 15:
        	  sync_delay = "0"
        	  Waveform = "'D:\Rohde-Schwarz\CMW\Data\Waveform\CaviumFRC_15MHz.wv'"
        elif self.cfg.cal_bandwidth == 10:
        	  sync_delay = "0"
        	  Waveform = "'D:\Rohde-Schwarz\CMW\Data\Waveform\CaviumFRC_10MHz.wv'"
        elif self.cfg.cal_bandwidth == 5:
        	  sync_delay = "0"
        	  Waveform = "'D:\Rohde-Schwarz\CMW\Data\Waveform\CaviumFRC_5MHz.wv'"
        
        print "VSG Setup..."
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:BBM ARB")
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:ARB:FILE " + Waveform)
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:ARB:REP CONT")
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:LIST OFF")
        type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:SOUR 'Manual'")
        type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:RETR ON")
        type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:AUT ON")
        print "Start VSG..."
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:RFS:FREQ " + str(freq_center * 1000000))
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:RFS:LEV " + str(self.init_pow))
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:STAT ON")
        type(self).cmw500.send_msg_to_server("*WAI")
        type(self).cmw500.send_msg_to_server("SOURce:GPRF:GEN:STATe?")
        in_msg = type(self).cmw500.recv_msg_frm_server()
        line = in_msg.split('\n')
        if (line[0] == "ON"):
            print "VSG turn on"
        else:
            print "VSG can not turn on"
            sys.exit()
                                  
               
    def start_enodb(self):
        
        type(self).enb.enb_login()
        type(self).enb.enb_cd_usr_bin()
        
        # default is PWM control in booting
        if (type(self).cfg.tcxo_ctrl == "dax"):
            print("load ext dac clock control")
            type(self).enb.enb_load_dax()   # do ext DAC control
    
        if test_config.band > 32:
            type(self).enb.enb_set_zen_tdd_rx()
            
        """
        if im_calibration.is_dsp_running:
            type(self).enb.enb_cd_tmpfs()
            type(self).enb.enb_stop_transmit()
        """
        
    def do_rssi_cal(self):
    
        #type(self).enb.enb_disable_all_TX()
        type(self).enb.enb_rf_drv_call()
        
        type(self).enb.tn_write("loss", "S")   # select rssi cal
        type(self).enb.tn_write("antenna", str(type(self).cfg.rssi_cable_loss))   # cable loss
        
        for ant in range(2):
            print('')
            print("Testing antenna port " + str(ant+1))
            type(self).enb.tn.write("\n")          # connect ant
            if (ant == 0):
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFAC, TX1")
            else:
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFBC, TX1")
            
            sleep(1)
            for cnt in range(0,4):
                type(self).enb.tn.read_until("press Enter")
                power = self.init_pow - 10*cnt
                print("EXG power = " + str(power) + ' dBm')
                
                if (type(self).cfg.test_set == 'agilent'):
                    type(self).exg.send_msg_to_server(':POW ' + str(power) + ' dBm')
                elif (type(self).cfg.test_set == 'anritsu'):
                    type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:RFS:LEV ' + str(power) + '\n')
                elif (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server('SOUR:GPRF:GEN:RFS:LEV ' + str(power))
                                
                sleep(0.5)    # wait RF power stable
                type(self).enb.tn.write("\n")
                
        try:
            res = type(self).enb.tn.read_until("calibration".encode("ascii"), 5)
            type(self).enb.tn_write("Option:", "n")
            type(self).enb.tn.write("q\n".encode("ascii"))
            
            start = 0
            while True:
                start = res.find("slope_1=".encode("ascii"), start)
                if (start == -1): break
                start += len("slope_1=")
                self.slope_1 = res[start:res.find(", ".encode("ascii"), start)]
                
                start = res.find("offset_1=".encode("ascii"), start)
                if (start == -1): break
                start += len("offset_1=")
                self.offset_1 = res[start:res.find('\n'.encode("ascii"), start)]
                
                start = res.find("slope_2=".encode("ascii"), start)
                if (start == -1): break
                start += len("slope_2=")
                self.slope_2 = res[start:res.find(", ".encode("ascii"), start)]
                
                start = res.find("offset_2=".encode("ascii"), start)
                if (start == -1): break
                start += len("offset_2=")
                self.offset_2 = res[start:res.find('\n'.encode("ascii"), start)]
                
                print("slope_1=" + self.slope_1 + ", offset_1=" + self.offset_1)
                print("slope_2=" + self.slope_2 + ", offset_2=" + self.offset_2)
                
                if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
                    self.rpt.write('\nRSSI Calibration:\n')
                    self.rpt.write("slope_1=" + self.slope_1 + ", offset_1=" + self.offset_1 + '\n')
                    self.rpt.write("slope_2=" + self.slope_2 + ", offset_2=" + self.offset_2 + '\n')
                
                break
            
            if (test_config.wr_var_to_uboot == True):
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv rssi_slope_prim " \
                                        + str(self.slope_1), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv rssi_offset_prim " \
                                        + str(self.offset_1), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv rssi_slope_sec " \
                                        + str(self.slope_2), 3)
                type(self).enb.tn_write(im_calibration.pp_base, "fsetenv rssi_offset_sec " \
                                        + str(self.offset_2), 3)
            
            if (type(self).cfg.en_eeprom_write == True):
                self.enb.editUbootenv('RSSI_SLOPE_PRIM', str(self.slope_1))
                self.enb.editUbootenv('RSSI_OFFSET_PRIM', str(self.offset_1))
                self.enb.editUbootenv('RSSI_SLOPE_SEC', str(self.slope_2))
                self.enb.editUbootenv('RSSI_OFFSET_SEC', str(self.offset_2))
                
                if (im_calibration.is_test_all == True):
                    self.write_rssi_to_eeprom()
                else:
                    se = raw_input("Write result to EEPROM?(y/n):")
                    if (se == 'y') or (se == 'Y'):
                        self.write_rssi_to_eeprom()
            
        except:
            print "Unexpected error:", sys.exc_info()[0]
        
        
    def write_rssi_to_eeprom(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        print "edit EEPROM record"
        type(self).enb.enb_eeprom_edit_record('wrsp', self.rn, self.slope_1)
        type(self).enb.enb_eeprom_edit_record('wrop', self.rn, self.offset_1)
        type(self).enb.enb_eeprom_edit_record('wrss', self.rn, self.slope_2)
        type(self).enb.enb_eeprom_edit_record('wros', self.rn, self.offset_2)
        sleep(0.5)    # wait EEPROM data wrote
        
    def exg_turn_off(self):
        
        type(self).exg.send_msg_to_server(':OUTP OFF')
        type(self).exg.send_msg_to_server(':OUTP:MOD OFF')
        
    def mt8870a_turn_off(self):
        
        type(self).mt8870a.send_msg_to_server('SOUR:GPRF:GEN:STAT OFF\n')

    def cmw500_turn_off(self):
        
        type(self).cmw500.send_msg_to_server('SOUR:GPRF:GEN:STAT OFF')
        
    def run(self):

        if (type(self).cfg.test_set == 'agilent'):
            self.exg_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(test_config.ul_freq)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(test_config.ul_freq)
	      
        
        self.start_enodb()
        print("**** enb started")
        self.do_rssi_cal()        
        
        print("**** rssi measure done")
        
        if (type(self).cfg.test_set == 'agilent'):
            self.exg_turn_off()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_turn_off()
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_turn_off()
	
        print ""
        
