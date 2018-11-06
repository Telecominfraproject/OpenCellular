#!/usr/bin/python

"""
Setup:
1. Agilent MXA, connect cable to one of the antenna port of enodeB
2. Set TFTP server, address and port as test_config
3. Test after TX Power calibration and reference clock calibration are done
"""

#import sys
import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration
#import common

class TestTxCfr(Calibration):
    
    def __init__(self, rpt_hndl, ch_gain, rf_gain, rf_shift_sat):
        
        self.rpt = rpt_hndl
        self.got_evm_pwr = False
        self.val_evm_pwr = 0.0
        
        self.res_txpwr = []
        self.res_evm = []
        self.optim_txpwr = []
        self.extra_att = 0 # extra attenuation to protect amplifier
        
        # CFR parameters
        self.ch_gain_addr = '0x10f000086ea08'
        self.rf_gain_addr = '0x10f000086ea0c'
        self.rf_shift_addr = '0x10f000086ea10'
        self.ch_gain_val = ch_gain
        self.rf_gain_val = rf_gain
        self.rf_shift_sat_val = rf_shift_sat
        # end of CFG parameters
        
        self.input_power = -10  # input power in dBm
        self.signal_bw = type(self).cfg.cal_bandwidth   # input signal bandwidth
        
        if (self.cfg.cal_bandwidth == 5):
            self.ltebw = 'B5M'
            self.intbw = '4.5'
            self.spabw = '10'
        elif (self.cfg.cal_bandwidth == 10):
            self.ltebw = 'B10M'
            self.intbw = '9'
            self.spabw = '20'
        elif (self.cfg.cal_bandwidth == 15):
            self.ltebw = 'B15M'
            self.intbw = '13.5'
            self.spabw = '25'
        elif (self.cfg.cal_bandwidth == 20):
            self.ltebw = 'B20M'
            self.intbw = '18'
            self.spabw = '30'
        
    def mxa_setup(self):
        
        type(self).mxa.send_msg_to_server('*RST')
        #type(self).mxa.send_msg_to_server('*PSC')
        type(self).mxa.send_msg_to_server(':INST LTE')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':DISP:MON:VIEW:WIND:TRAC:Y:RLEV 20')   # reference level
        type(self).mxa.send_msg_to_server(':POW:ATT 40')               # attenuation
        type(self).mxa.send_msg_to_server(':RAD:STAN:BAND ' + self.ltebw)   # LTE bandwidth
        type(self).mxa.send_msg_to_server(':MON:FREQ:SPAN ' + self.spabw + ' MHz')     # frequency span
        type(self).mxa.send_msg_to_server(':RAD:STAN:DIR DLIN')        # downlink
        type(self).mxa.send_msg_to_server(':CONF:CEVM')
        #type(self).mxa.send_msg_to_server(':CONF:EVM')
        type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:ANT:NUMB ANT2')
            
    def set_mxa_chp_measure(self):
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        #type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':INST:SEL LTE')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + str(test_config.dl_freq) + ' MHz')
        type(self).mxa.send_msg_to_server(':CONF:CHP')
        type(self).mxa.send_msg_to_server(':POW:ATT 40')
        type(self).mxa.send_msg_to_server(':DISP:CHP:VIEW:WIND:TRAC:Y:RLEV 20 dBm')
        #type(self).mxa.send_msg_to_server(':CHP:BAND:INT ' + self.intbw + ' MHz')
        #type(self).mxa.send_msg_to_server(':CHP:FREQ:SPAN ' + self.spabw + ' MHz')
        #type(self).mxa.send_msg_to_server(':CHP:AVER:COUN 10')
        #type(self).mxa.send_msg_to_server(':CHP:AVER ON')
        type(self).mxa.send_msg_to_server(':RAD:STAN:PRES ' + self.ltebw)   # LTE bandwidth
        type(self).mxa.send_msg_to_server(':MON:FREQ:SPAN ' + self.spabw + ' MHz')
        
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
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:FREQ " + str(freq_center) + " MHz")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:ENP 35")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:RFS:UMAR 0")
        
        if (self.cfg.cal_bandwidth == 5):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B050")
        elif (self.cfg.cal_bandwidth == 10):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B100")
        elif (self.cfg.cal_bandwidth == 15):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B150")
        elif (self.cfg.cal_bandwidth == 20):
            type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:CBAN B200")
        
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:RES:ALL ON,OFF,OFF,OFF,OFF,OFF,ON,OFF,OFF")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:SCO:MOD 10")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:MOEX ON")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:REP SING")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:ETES ETM11")
        type(self).cmw500.send_msg_to_server("CONF:LTE:MEAS:ENB:MEV:PLC 1")
        type(self).cmw500.send_msg_to_server("TRIG:LTE:MEAS:ENB:MEV:SOUR 'Free Run (Fast Sync)'")
        
    def start_enodeb_tx(self):
        
        type(self).enb.enb_login()
        
        # default is PWM control in booting
        if (type(self).cfg.tcxo_ctrl == "dax"):
            print("load ext dac clock control")
            type(self).enb.enb_cd_usr_bin()
            type(self).enb.enb_load_dax()   # do ext DAC control
    
        if test_config.band > 32:
            type(self).enb.enb_cd_usr_bin()
            type(self).enb.enb_set_zen_tdd_tx()
        
        self.get_optimized_txpwr_record()
        self.set_extra_attenuation()    
        
        # set CFR parameters
        #type(self).enb.remote_memory_32bit_set(1, self.ch_gain_addr, self.ch_gain_val)
        #type(self).enb.remote_memory_32bit_set(1, self.rf_gain_addr, self.rf_gain_val)
        #type(self).enb.remote_memory_32bit_set(1, self.rf_shift_addr, self.rf_shift_sat_val)
            
        type(self).enb.enb_cd_tmpfs()
        type(self).enb.enb_run_dsp_app_dl()
        
        #type(self).enb.enb_cd_usr_bin()
        # set CFR parameters
        type(self).enb.remote_memory_32bit_set(1, self.ch_gain_addr, self.ch_gain_val)
        type(self).enb.remote_memory_32bit_set(1, self.rf_gain_addr, self.rf_gain_val)
        type(self).enb.remote_memory_32bit_set(1, self.rf_shift_addr, self.rf_shift_sat_val)
    
    def get_optimized_txpwr_record(self):
        
        type(self).enb.enb_cd_usr_bin()
        
        # primary tx power
        type(self).enb.tn.write("oncpu 0 /usr/bin/" + \
                                type(self).cfg.rf_driver + " <<  EOF\n")
        type(self).enb.tn.write("e rrc 1 rmpp q\n")
        type(self).enb.tn.write("EOF\n")
        
        _ = type(self).enb.tn.read_until("rec_version".encode("ascii"), 5)
        res = type(self).enb.tn.read_until("/usr/bin".encode("ascii"), 5)
        
        start = 0
        start = res.find("tx_max_pwr_prim = ".encode("ascii"), start)
        start += len("tx_max_pwr_prim = ")
        pwr1 = res[start:res.find('\n'.encode("ascii"), start)]
        
        # secondary tx power
        type(self).enb.tn.write("oncpu 0 /usr/bin/" + \
                                type(self).cfg.rf_driver + " <<  EOF\n")
        type(self).enb.tn.write("e rrc 1 rmps q\n")
        type(self).enb.tn.write("EOF\n")
        
        _ = type(self).enb.tn.read_until("rec_version".encode("ascii"), 5)
        res = type(self).enb.tn.read_until("/usr/bin".encode("ascii"), 5)
        
        start = 0
        start = res.find("tx_max_pwr_sec = ".encode("ascii"), start)
        start += len("tx_max_pwr_sec = ")
        pwr2 = res[start:res.find('\n'.encode("ascii"), start)]
        
        self.optim_txpwr.append(round(float(pwr1.strip()), 2))
        self.optim_txpwr.append(round(float(pwr2.strip()), 2))
        print "pwr1=" + str(self.optim_txpwr[0]) + ", pwr2=" + str(self.optim_txpwr[1])
    
    def set_extra_attenuation(self):
        
        cmd = "t 1 " + str(int(self.optim_txpwr[0]) - self.extra_att) + \
            " t 2 " + str(int(self.optim_txpwr[1]) - self.extra_att) + " q"
            
        type(self).enb.enb_rf_drv_call_cmd(cmd)
    
    # Find max TX power and check EVM
    def get_tx_cfr_evm(self):
        
        if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
            self.rpt.write('\nTX Max Available Power:\n')
        
        for ant in range(2):
            print('')
            print("Testing antenna port " + str(ant+1))
            
            if (ant == 0):
                type(self).enb.enb_disable_TX2()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFAC, RX1")
            else:
                type(self).enb.enb_disable_TX1()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFBC, RX1")
            
            sleep(3)
                    
            if (type(self).cfg.test_set == 'agilent'):
                #type(self).enb.enb_enable_all_TX()
                self.mxa_setup()
                if (ant == 0):
                    type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:ANT:PORT P0')
                    type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:SS:ANT:PORT P0')
                else:
                    type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:ANT:PORT P1')
                    type(self).mxa.send_msg_to_server('EVM:CCAR0:DLIN:SYNC:SS:ANT:PORT P1')
                sleep(2)
                
                type(self).mxa.send_msg_to_server(':READ:CEVM?')
                #type(self).mxa.send_msg_to_server(':READ:EVM?')
                in_msg = type(self).mxa.recv_msg_frm_server()
                #print('recv ' + type(self).cfg.mxa_ipaddr + '= ' + in_msg)
                
                line = in_msg.split(',')
                evm = round(float(line[0]), 2)
                txpwr = round(float(line[11]), 2)
            
            elif (type(self).cfg.test_set == 'anritsu'):
                """
                type(self).mt8870a.send_msg_to_server(':INIT:MODE:SING')
                type(self).mt8870a.send_msg_to_server('*WAI')
                type(self).mt8870a.send_msg_to_server(':FETC:BATC1?')
                type(self).mt8870a.send_msg_to_server('*WAI')
                in_msg = type(self).mt8870a.recv_msg_frm_server()
                print in_msg
                line = in_msg.split(',')
                """
                evm = round(float(line[3]), 2)
                txpwr = round(float(line[11]), 2) #TODO: need to check
                
            elif (type(self).cfg.test_set == 'rs'):
                    
                    type(self).cmw500.send_msg_to_server("INIT:LTE:MEAS:ENB:MEV")
                    type(self).cmw500.send_msg_to_server("*WAI")
                    type(self).cmw500.send_msg_to_server("FETC:LTE:MEAS:ENB:MEV:MOD:AVER?")
                    in_msg = type(self).cmw500.recv_msg_frm_server()
                    line = in_msg.split(',')
                    if (int(line[0]) != 0):
                        print in_msg
                    evm = round(float(line[3]), 2)
                    txpwr = round(float(line[17]), 2)  
                    
                
            self.res_evm.append(evm)
            self.res_txpwr.append(txpwr + type(self).cfg.txpwr_cable_loss)
            
        #print('')
        print "Channel Gain Shift Off Value = " + str(self.ch_gain_val)
        print "RF Gain Control = " + str(self.rf_gain_val)
        print "RF Shift Value = " + str(self.rf_shift_sat_val)
        print "ANT1, EVM=" + str(self.res_evm[0]) + "%, TX PWR=" + \
                str(self.res_txpwr[0]) + "dBm"
        print "ANT2, EVM=" + str(self.res_evm[1]) + "%, TX PWR=" + \
                str(self.res_txpwr[1]) + "dBm"
        print ""
        
        if (type(self).cfg.test_report == True):
            self.rpt.write("Channel Gain Shift Off Value = " + str(self.ch_gain_val) + "\n")
            self.rpt.write("RF Gain Control = " + str(self.rf_gain_val) + "\n")
            self.rpt.write("RF Shift Value = " + str(self.rf_shift_sat_val) + "\n")
            self.rpt.write("ANT1, EVM=" + str(self.res_evm[0]) + "%, TX PWR=" + \
                           str(self.res_txpwr[0]) + "dBm\n")
            self.rpt.write("ANT2, EVM=" + str(self.res_evm[1]) + "%, TX PWR=" + \
                           str(self.res_txpwr[1]) + "dBm\n\n")
            self.rpt.flush()

    def run(self):
        
        self.start_enodeb_tx()
        
        if (type(self).cfg.test_set == 'agilent'):
            self.mxa_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(test_config.dl_freq)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(test_config.dl_freq)
            
        sleep(3)
        
        #self.set_mxa_chp_measure()
        self.get_tx_cfr_evm()
        
