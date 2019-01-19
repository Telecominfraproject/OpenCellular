#!/usr/bin/python

import tcp_client
import enodeb_ctrl
import test_config
from time import sleep

# global variable
is_dsp_running = False              # DSP running indicator
is_test_all = False                 # test all items
pp_base = '>'   # prompt base for tn_write

class Calibration(object):
    
    cfg = test_config.EnbConfig()
    cfg.set_test_equipment()
    enb = enodeb_ctrl.enodeB_Ctrl()
    
    if (cfg.test_set == 'agilent'):
        mxa = tcp_client.TcpClient(cfg.mxa_ipaddr, cfg.mxa_tcp_port)
        exg = tcp_client.TcpClient(cfg.exg_ipaddr, cfg.exg_tcp_port)
    elif (cfg.test_set == 'anritsu'):
        mt8870a = tcp_client.TcpClient(cfg.mt8870a_ipaddr, cfg.mt8870a_tcp_port)
    elif (cfg.test_set == 'rs'):
        cmw500 = tcp_client.TcpClient(cfg.cmw500_ipaddr, cfg.cmw500_tcp_port)
    
    dl_freq = test_config.dl_freq
    ul_freq = test_config.ul_freq
    
    def __init__(self, rpt_hndl, rec_num):
        
        self.rpt = rpt_hndl
        self.rn = rec_num
    
    def exg_connect(self):

        type(self).exg.tcp_connect()
        sleep(1)
    
    def exg_init(self):
        
        type(self).exg.send_msg_to_server('*IDN?')
        in_msg = type(self).exg.recv_msg_frm_server()
        print('recv ' + type(self).cfg.exg_ipaddr + '= ' + in_msg)
    
    def exg_setup(self):
        pass
    
    def mxa_connect(self):
        
        type(self).mxa.tcp_connect()
        sleep(1)
        
    def mxa_init(self):
        
        if (type(self).cfg.instr_disp == True):
            type(self).mxa.send_msg_to_server(":DISPlay:ENABle ON")
        else: 
            type(self).mxa.send_msg_to_server(":DISPlay:ENABle OFF")

        type(self).mxa.send_msg_to_server('*RST')
        type(self).mxa.send_msg_to_server('*IDN?')
        in_msg = type(self).mxa.recv_msg_frm_server()
        print('Instrument ID: ' + in_msg)     
        
    def mxa_pre_setup(self):
        pass
        
    def mxa_setup(self, freq_center):
        
        type(self).mxa.send_msg_to_server(':INIT:CONT ON')
        type(self).mxa.send_msg_to_server(':INST:SEL SA')
        type(self).mxa.send_msg_to_server(':FREQ:CENT ' + 
                                str(freq_center) + ' MHz')
        type(self).mxa.send_msg_to_server(':FREQ:SPAN ' +
                                str(type(self).cfg.cal_bandwidth+10) + ' MHz')
        type(self).mxa.send_msg_to_server(':POW:ATT 30')
        
    def mt8870a_connect(self):

        type(self).mt8870a.tcp_connect()
        sleep(1)
    
    def mt8870a_init(self):
        
        type(self).mt8870a.send_msg_to_server('*IDN?')
        in_msg = type(self).mt8870a.recv_msg_frm_server()
        print('Instrument ID: ' + in_msg)
        
        print 'Instrument Setup...\n'
        type(self).mt8870a.send_msg_to_server('SYST:LANG SCPI')
        type(self).mt8870a.send_msg_to_server('INST SMALLCELL')
        type(self).mt8870a.send_msg_to_server('INST:SYST 3GLTE_DL,ACT')
        type(self).mt8870a.send_msg_to_server(':ROUT:PORT:CONN:DIR PORT3,PORT4')
        
    def mt8870a_pre_setup(self):
        pass
        
    def mt8870a_setup(self, freq_center):
        pass
        
    def cmw500_connect(self):
        
        type(self).cmw500.tcp_connect()
        sleep(1)
        
    def cmw500_init(self):
        type(self).cmw500.send_msg_to_server("*RST")
        type(self).cmw500.send_msg_to_server("*IDN?")
        in_msg = type(self).cmw500.recv_msg_frm_server()
        print('Instrument ID: ' + in_msg)
        
        print 'Instrument Setup...\n'
        if (type(self).cfg.instr_disp == True):
            type(self).cmw500.send_msg_to_server("SYSTem:DISPlay:UPDate ON")
            type(self).cmw500.send_msg_to_server("*GTL")
        else: 
            type(self).cmw500.send_msg_to_server("SYSTem:DISPlay:UPDate OFF")
            type(self).cmw500.send_msg_to_server("*GTL")
            
        type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFAC, TX1")
        type(self).cmw500.send_msg_to_server("ROUTe:GPRF:MEASurement:SCENario:SALone RFAC, RX1")
        type(self).cmw500.send_msg_to_server("ROUTe:LTE:MEAS:ENB:SCENario:SALone RFAC, RX1")
        
    def cmw500_pre_setup(self):
        pass
        
    def cmw500_setup(self, freq_center):
        pass
        
    def start_enodeb_tx(self):
        # start enodeB
        type(self).enb.start_telnet_session()
        type(self).enb.enb_login()
        type(self).enb.get_macaddr()
        type(self).enb.enb_set_1pps()

        type(self).enb.enb_cd_usr_bin()
        type(self).enb.enb_load_rf_drv()
        type(self).enb.enb_load_rf_init()

        type(self).enb.enb_set_rf_drv_rf_card(test_config.dl_freq[0], test_config.ul_freq[0])
        
        if (type(self).cfg.board_typ == "zen_ad"):
            print "load pwm clock control"
            type(self).enb.enb_load_pwm()
        else:
            print "load dac clock control"
            type(self).enb.enb_load_dac()      # do DAC control
            
        if test_config.band > 32:
            type(self).enb.enb_set_zen_tdd_tx()
            
        type(self).enb.enb_cd_etc()
        type(self).enb.enb_load_cazac()
        type(self).enb.enb_load_calgrant()
        
        type(self).enb.enb_cd_tmpfs()
        type(self).enb.enb_load_LSM_X_L1_0_July10()
        type(self).enb.enb_load_dsp_app_dl()
        type(self).enb.enb_set_dsp_app_dl()
        type(self).enb.enb_set_7ffeff00()
        
        type(self).enb.enb_cleanup_tmpfs_partition()
        type(self).enb.enb_cd_tmpfs()
        type(self).enb.enb_load_dsp_app_dl()
        type(self).enb.enb_load_LSM_X_TV_0_wk21_00_ETM()
        type(self).enb.enb_run_dsp_app_etm1p1()
        
    def end_sys(self):
        
        print("")
        #type(self).mxa.send_msg_to_server(":DISPlay:ENABle ON")       # turn on display
        
        if (self.cfg.test_set == 'agilent'):
            type(self).exg.tcp_close()
            type(self).mxa.tcp_close()
        elif (self.cfg.test_set == 'anritsu'):
            type(self).mt8870a.tcp_close()
        elif (self.cfg.test_set == 'rs'):
            type(self).cmw500.tcp_close()
            
        #type(self).enb.end_telnet_session()
        