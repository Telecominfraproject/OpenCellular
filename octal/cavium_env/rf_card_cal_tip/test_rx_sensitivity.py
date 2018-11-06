#!/usr/bin/python
import common
import sys
import udp_server
import test_config
import im_calibration
from time import sleep
from im_calibration import Calibration

class RxSens(Calibration):
    
    def __init__(self, rpt_hndl):
        
        self.rpt = rpt_hndl
        self.init_pow = -60     # ESG test from this amplitude
        self.udpsvr = ''
        self.curr_pow = 0
        self.sens_pow = []
        self.loop_a = 5
        self.loop_b = 10
        self.per_pass = False
        self.isFirstGetPer = True
        self.zero_cnt = 0
        self.coarse_pass_cnt = 0
        
    def exg_setup(self):
        
        type(self).exg.send_msg_to_server('*RST')
        type(self).exg.send_msg_to_server('*IDN?')
        in_msg = type(self).exg.recv_msg_frm_server()
        print('recv ' + type(self).cfg.exg_ipaddr + '= ' + in_msg)
        
        #type(self).exg.send_msg_to_server(':RAD:ARB ON')
        type(self).exg.send_msg_to_server(':RAD:ARB:WAV ' + type(self).cfg.exg_waveform)
        print("EXG waveform " + type(self).cfg.exg_waveform)
        
        if self.cfg.cal_bandwidth == 20:
            type(self).exg.send_msg_to_server(':RAD:ARB:SCL:RATE 30720000')
        elif (self.cfg.cal_bandwidth == 15):
            type(self).exg.send_msg_to_server(':RAD:ARB:SCL:RATE 23040000')
        elif (self.cfg.cal_bandwidth == 10):
            type(self).exg.send_msg_to_server(':RAD:ARB:SCL:RATE 15360000')
        elif (self.cfg.cal_bandwidth == 5):
            type(self).exg.send_msg_to_server(':RAD:ARB:SCL:RATE 7680000')
            
        type(self).exg.send_msg_to_server(':FREQ ' + str(test_config.ul_freq) + ' MHz')
        type(self).exg.send_msg_to_server(':POW ' + str(self.init_pow) + ' dBm')
        type(self).exg.send_msg_to_server(':OUTP:MOD ON')
        type(self).exg.send_msg_to_server(':OUTP ON')
        
        #type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:TYPE:CONT TRIG')
        type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:TYPE:CONT RES')
        type(self).exg.send_msg_to_server(':RAD:ARB:TRIG EXT')
        type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:EXT:DEL:STAT ON')
        
        if self.cfg.cal_bandwidth == 20:
            sync_delay = '0.009947'
        elif self.cfg.cal_bandwidth == 10:
            sync_delay = '0.009900'
        elif self.cfg.cal_bandwidth == 15:
            sync_delay = '0.009930'
        elif self.cfg.cal_bandwidth == 5:
            sync_delay = '0.009807'
        
        type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:EXT:DEL ' + 
                                          sync_delay) # EXT delay
        #type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:EXT:DEL ON')
        type(self).exg.send_msg_to_server(':RAD:ARB ON')
        
    def exg_sync_trigger(self):
        
        #type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:TYPE:CONT TRIG')
        type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:TYPE:CONT RES')
        
    def mt8870a_setup(self, powVal):
        
        Waveform = ""
        if self.cfg.cal_bandwidth == 20:
            sync_delay = '9.99601'
            Waveform = "'SmallCell_LTEFDD_FRC_A1-3_20M_163_61_1_HARQ-ACK'"
        elif self.cfg.cal_bandwidth == 15:
            sync_delay = '9.99680' #TODO: need to chect
            Waveform = "'SmallCell_LTEFDD_FRC_A1-3_15M_163_61_1_HARQ-ACK'"
        elif self.cfg.cal_bandwidth == 10:
            sync_delay = '9.99770'
            Waveform = "'SmallCell_LTEFDD_FRC_A1-3_10M_163_61_1_HARQ-ACK'"
        elif self.cfg.cal_bandwidth == 5:
            sync_delay = '9.99857'
            Waveform = "'SmallCell_LTEFDD_FRC_A1-3_5M_163_61_1_HARQ-ACK'"
        
        #print 'MT8870A setup renew\n'
        type(self).mt8870a.send_msg_to_server('*RST\n')
        type(self).mt8870a.send_msg_to_server('SYST:LANG SCPI')
        type(self).mt8870a.send_msg_to_server(':ROUT:PORT:CONN:DIR PORT3,PORT4')
        #print 'MT8870A test setup\n'
        type(self).mt8870a.send_msg_to_server(':INST SMALLCELL\n')
        type(self).mt8870a.send_msg_to_server(':INST:SYST SG,ACT\n')
        type(self).mt8870a.send_msg_to_server(':SOURce:GPRF:GEN:ARB:FILE:LOAD ' + Waveform)
        type(self).mt8870a.send_msg_to_server('*WAI\n')
        type(self).mt8870a.send_msg_to_server(':RAD:ARB:WAV ' + Waveform + ',1\n')
        type(self).mt8870a.send_msg_to_server(':FREQ ' + str(test_config.ul_freq)  + 'MHZ\n')
        type(self).mt8870a.send_msg_to_server(':POW ' + str(powVal) + 'DBM\n')
        type(self).mt8870a.send_msg_to_server(':OUTP ON\n')
        type(self).mt8870a.send_msg_to_server(':OUTP:MOD ON\n')
        type(self).mt8870a.send_msg_to_server(':SOURce:GPRF:GEN:ARB:TRIG ON\n')
        type(self).mt8870a.send_msg_to_server(':SOURce:GPRF:GEN:ARB:TRIG:DEL ' + 
                                              sync_delay + 'ms\n')
        type(self).mt8870a.send_msg_to_server(':SOURce:GPRF:GEN:ARB:TRIG:SLOP RISE\n')
        type(self).mt8870a.send_msg_to_server(':SOURce:GPRF:GEN:ARB:WAV:REST\n')
        type(self).mt8870a.send_msg_to_server(':INST:SYST 3GLTE_DL,ACT\n')
        
    def cmw500_setup(self, powVal):
        
        Waveform = ""
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
        type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:SOUR 'Base1: External TRIG A'")
        type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:DEL " + sync_delay)
        type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:RETR ON")
        type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:AUT ON")
        print "Start VSG..."
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:RFS:FREQ " + str(test_config.ul_freq)  + "MHz")
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:RFS:LEV " + str(powVal))
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
        
    def start_enodb_rx(self):
        
        #type(self).enb.start_telnet_session()
        type(self).enb.enb_login()
        type(self).enb.enb_cd_usr_bin()
        """
        type(self).enb.enb_load_rf_driver() #TODO: remove after new rf driver tested
        type(self).enb.enb_run_lsmLogDisp()
        """
        #if test_config.band > 32:
        #    type(self).enb.enb_set_rfif_hab('rx')
        
        type(self).enb.rx_set_rf_drv_rf_card()
        
        if test_config.band > 32:
            type(self).enb.enb_disable_all_TX()
            type(self).enb.enb_set_zen_tdd_rx()
        
        #type(self).enb.enb_set_7ffeff00()
        type(self).enb.enb_load_lfmsoft()
        #type(self).enb.rx_load_test_vector()
        #sleep(10)
        
        #TODO: remove after merge in kernel
        #type(self).enb.enb_cd_etc()
        #type(self).enb.enb_load_dsp_to_etc
        #type(self).enb.enb_cd_usr_bin()
        
        print "running DSP..."
        type(self).enb.enb_cd_tmpfs()
        #type(self).enb.rx_display_bler()
        type(self).enb.rx_run_dsp_app()
        sleep(5)
         
        sync_val = type(self).enb.get_delay_sync_value()
        
        if (sync_val < 50) and (sync_val >= 0):
            if (type(self).cfg.test_set == 'agilent'):
                if self.cfg.cal_bandwidth == 20:
                    sync_val = round(float((9.943 + sync_val)/1000), 6)
                elif self.cfg.cal_bandwidth == 15:
                    sync_val = round(float((9.930 + sync_val)/1000), 6)
                elif self.cfg.cal_bandwidth == 10:
                    sync_val = round(float((9.900 + sync_val)/1000), 6)
                elif self.cfg.cal_bandwidth == 5:
                    sync_val = round(float((9.807 + sync_val)/1000), 6)
                else:
                    sync_val = 0.009900
                type(self).exg.send_msg_to_server(':RAD:ARB:TRIG:EXT:DEL ' + str(sync_val))
            
            elif (type(self).cfg.test_set == 'anritsu'):
                if self.cfg.cal_bandwidth == 20:
                    sync_delay = '9.99601'
                elif self.cfg.cal_bandwidth == 15:
                    sync_delay = '9.99680'
                elif self.cfg.cal_bandwidth == 10:
                    sync_delay = '9.99770'
                elif self.cfg.cal_bandwidth == 5:
                    sync_delay = '9.99857'
                type(self).mt8870a.send_msg_to_server(':SOURce:GPRF:GEN:ARB:TRIG:DEL ' + 
                                              sync_delay + 'ms\n')
                          
            elif (type(self).cfg.test_set == 'rs'):
                if self.cfg.cal_bandwidth == 20:
                    sync_delay = '0'
                elif self.cfg.cal_bandwidth == 15:
                    sync_delay = '0'
                elif self.cfg.cal_bandwidth == 10:
                    sync_delay = '0'
                elif self.cfg.cal_bandwidth == 5:
                    sync_delay = '0'
                type(self).cmw500.send_msg_to_server("TRIG:GPRF:GEN:ARB:DEL " + sync_delay)
                
        
    def exg_turn_off(self):
        
        type(self).exg.send_msg_to_server(':OUTP OFF')
        type(self).exg.send_msg_to_server(':OUTP:MOD OFF')
        
        
    def mt8870a_turn_off(self):
        
        type(self).mt8870a.send_msg_to_server(':SOUR:GPRF:GEN:STAT 0')
        
    def cmw500_turn_off(self):
        
        type(self).cmw500.send_msg_to_server("SOUR:GPRF:GEN:STAT OFF")
        
    def run(self):
            
        if (type(self).cfg.test_set == 'agilent'):
            self.exg_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(self.init_pow)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(self.init_pow)
        """
        try:
            self.udpsvr = udp_server.UdpServer(type(self).cfg.my_udp_ipaddr, 
                                           type(self).cfg.my_udp_port)
            self.udpsvr.start_socket()
        except:
            return -1
        """
        self.start_enodb_rx()
        #common.hit_continue() # pause for logparser test
        
        # wait until throughput information comes out
        detect_loop = 5
        for cn in range(0, detect_loop):
            try:
                self.mesg = self.udpsvr.recv_frm_client()
                start = self.mesg.find("Tput".encode("ascii"), 0)
                if (start > 0):
                    print "found throughput information"
                    break
                if (cn == (detect_loop-1)):
                    print "no throughput message found"
                    """
                    self.udpsvr.udp_close()
                    if (type(self).cfg.test_set == 'agilent'):
                        self.exg_turn_off()
                    elif (type(self).cfg.test_set == 'anritsu'):
                        self.mt8870a_turn_off()
                    self.end_sys(1)
                    """
                    sys.exit()
            except:
                pass
                #print "RF driver message skipped"
                
        # clear buffer
        for _ in range(10):
            s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"), 0.5)
                
        self.loop_a = (120 + self.init_pow)/10
        type(self).enb.enb_cd_usr_bin()
        for ant in range(2):
            
            print("\nTesting antenna port " + str(ant+1))
            
            if (ant == 0):
                type(self).enb.enb_disable_RX2()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFAC, TX1")
                    
            else:
                type(self).enb.enb_disable_RX1()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFBC, TX1")
                sleep(5)
            #common.hit_continue("check gain registers...")
                
            s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"), 0.5)
            
            # coarse search
            for cn in range(0, self.loop_a):
                
                self.per_pass = False
                self.curr_pow = self.init_pow - 10*cn
                print("\nVSG power = " + str(self.curr_pow) + ' dBm')
                
                if (type(self).cfg.test_set == 'agilent'):
                    type(self).exg.send_msg_to_server(':POW ' + str(self.curr_pow) + ' dBm')
                    self.exg_sync_trigger()
                elif (type(self).cfg.test_set == 'anritsu'):
                    self.mt8870a_setup(self.curr_pow)
                elif (type(self).cfg.test_set == 'rs'):
                    self.cmw500_setup(self.curr_pow)
                
                sleep(3)    # wait RX demodulation stable
                
                self.zero_cnt = 0
                self.coarse_pass_cnt = 0
                self.isFirstGetPer = True
                for _ in range(0, self.loop_b):

                    per, tput = self.get_per_tput()
                    #per, tput = self.getPerFromLogParser()
                    
                    if tput >= 0:
                        print 'PER ' + str(per) + '% Tput ' + str(tput)
                        print ''
                        
                    if (per <= (type(self).cfg.bler_limit)):
                        if (self.coarse_pass_cnt >= 3):
                            print "VSG power " + str(self.curr_pow) + " dBm passed"
                            self.per_pass = True
                            break
                        else:
                            self.coarse_pass_cnt += 1
                            continue
                    
                    if (tput == 0):
                        self.zero_cnt = self.zero_cnt + 1
                    
                    if (self.zero_cnt == 3):
                        print "go next"
                        break
                    
                if (self.per_pass == False):
                    if (cn == 0):
                        print "RX sensitivity test failed"
                        #self.udpsvr.udp_close()
                        if (type(self).cfg.test_set == 'agilent'):
                            self.exg_turn_off()
                        elif (type(self).cfg.test_set == 'anritsu'):
                            self.mt8870a_turn_off()
                        elif (type(self).cfg.test_set == 'rs'):
                            self.cmw500_turn_off()
                        #self.end_sys()
                        return
                        #sys.exit()
                    else:
                        break
    
            # fine search
            loop_fine_search = 20
            loop_each_level = 10
            for cs in range(0, loop_fine_search):
                
                self.per_pass = False
                self.curr_pow = self.curr_pow + 1
                print("\nVSG power = " + str(self.curr_pow) + ' dBm')
                
                if (type(self).cfg.test_set == 'agilent'):
                    #print ':POW ' + str(self.curr_pow) + ' dBm'
                    type(self).exg.send_msg_to_server(':POW ' + str(self.curr_pow) + ' dBm')
                    #print "set EXG sync trigger"
                    self.exg_sync_trigger()
                elif (type(self).cfg.test_set == 'anritsu'):
                    self.mt8870a_setup(self.curr_pow)
                elif (type(self).cfg.test_set == 'rs'):
                    self.cmw500_setup(self.curr_pow)
                
                sleep(3)    # wait RF power stable
                
                self.zero_cnt = 0
                self.isFirstGetPer = True
                for _ in range(0, loop_each_level):
                    
                    per, tput = self.get_per_tput()
                    #print "receiving PER information from logparser..."
                    #per, tput = self.getPerFromLogParser()
                    
                    if tput >= 0:
                        print 'PER ' + str(per) + '% Tput ' + str(tput)
                        print ''
                    if (per < (type(self).cfg.bler_limit)):
                        print "VSG power " + str(self.curr_pow) + " dBm passed"
                        sen = self.curr_pow - type(self).cfg.rssi_cable_loss
                        self.sens_pow.append(sen)
                        self.per_pass = True
                        break
                    
                    if (tput == 0):
                        self.zero_cnt = self.zero_cnt + 1
                    
                    if (self.zero_cnt == 3):
                        print "go next"
                        break
                
                if (self.per_pass == True):
                    break
                
                if (cs == 19):
                    print "Sensitivity test failed"
                    sys.exit()
        
        print ''
        print "RX1 sensitivity = " + str(self.sens_pow[0]) + " dBm\n"
        print "RX2 sensitivity = " + str(self.sens_pow[1]) + " dBm\n"
        
        #self.udpsvr.udp_close()
        if (type(self).cfg.test_set == 'agilent'):
            self.exg_turn_off()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_turn_off()
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_turn_off()
        return self.sens_pow
        
        
    def run_limit_test(self):
        
        if (type(self).cfg.test_set == 'agilent'):
            self.exg_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(self.init_pow)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(self.init_pow)
        """
        try:
            self.udpsvr = udp_server.UdpServer(type(self).cfg.my_udp_ipaddr, 
                                           type(self).cfg.my_udp_port)
            self.udpsvr.start_socket()
        except:
            return -1
        """
        self.start_enodb_rx()
        print "waiting for result...."
        
        # wait until throughput comes out
        detect_loop = 5
        for cn in range(0, detect_loop):
            try:
                #self.mesg = self.udpsvr.recv_frm_client()
                #print self.mesg
                start = self.mesg.find("Tput".encode("ascii"), 0)
                if (start > 0):
                    print "found throughput information"
                    break
                if (cn == (detect_loop - 1)):
                    print "no throughput message found"
                    """
                    self.udpsvr.udp_close()
                    
                    if (type(self).cfg.test_set == 'agilent'):
                        self.exg_turn_off()
                    elif (type(self).cfg.test_set == 'anritsu'):
                        self.mt8870a_turn_off()
                    
                    self.end_sys(1)
                    """
                    sys.exit()
            except:
                pass
                #print "RF driver message skipped"
                
        # clear buffer
        for _ in range(10):
            s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"), 0.5)
        
        type(self).enb.enb_cd_usr_bin()
        for ant in range(2):
            
            per = 100.0
            tput = 0.0
            print("Testing antenna port " + str(ant+1))
            
            if (ant == 0):
                type(self).enb.enb_disable_RX2()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFAC, TX1")
            else:
                type(self).enb.enb_disable_RX1()
                if (type(self).cfg.test_set == 'rs'):
                    type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFBC, TX1")
                sleep(5)
                
            s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"), 0.5)
            
            # test limit
            self.per_pass = False
            self.curr_pow = type(self).cfg.sens_pass_limit + type(self).cfg.rssi_cable_loss
            print("\nVSG power = " + str(self.curr_pow) + ' dBm')
            
            if (type(self).cfg.test_set == 'agilent'):
                type(self).exg.send_msg_to_server(':POW ' + str(self.curr_pow) + ' dBm')
                self.exg_sync_trigger()
            elif (type(self).cfg.test_set == 'anritsu'):
                self.mt8870a_setup(self.curr_pow)
            elif (type(self).cfg.test_set == 'rs'):
                self.cmw500_setup(self.curr_pow)
            
            sleep(3)    # wait RF power stable
            #self.clear_telnet_message()
            
            self.isFirstGetPer = True
            for _ in range(0, 10):
                
                per, tput = self.get_per_tput()
                #per, tput = self.getPerFromLogParser()
                
                if tput >= 0:
                    print 'PER ' + str(per) + '% Tput ' + str(tput)
                    print ''
                if (per < (type(self).cfg.bler_limit)):
                    print "VSG power " + str(self.curr_pow) + " dBm passed"
                    self.per_pass = True
                    break
            
            if (self.per_pass == True):
                self.sens_pow.append("PASS")
            else:
                self.sens_pow.append("FAIL")
        
        print ''
        print "RX1 " + str(type(self).cfg.sens_pass_limit) + " dBm:" + self.sens_pow[0] + "\n"
        print "RX2 " + str(type(self).cfg.sens_pass_limit) + " dBm:" + self.sens_pow[1] + "\n"
        
        if (type(self).cfg.test_report == True) and (im_calibration.is_test_all == True):
            self.rpt.write('\nSensitivity:\n')
            self.rpt.write("RX1 " + str(type(self).cfg.sens_pass_limit) + " dBm:" + self.sens_pow[0] + "\n")
            self.rpt.write("RX2 " + str(type(self).cfg.sens_pass_limit) + " dBm:" + self.sens_pow[1] + "\n")
        
        #self.udpsvr.udp_close()
        if (type(self).cfg.test_set == 'agilent'):
            self.exg_turn_off()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_turn_off()
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_turn_off()
        
        return self.sens_pow


    def run_continuous(self):
        
        if (type(self).cfg.test_set == 'agilent'):
            self.exg_setup()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(self.init_pow)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(self.init_pow)
            
        self.start_enodb_rx()
        print "waiting for result...."
        
        # wait until throughput comes out
        detect_loop = 5
        for cn in range(0, detect_loop):
            try:
                #self.mesg = self.udpsvr.recv_frm_client()
                #print self.mesg
                start = self.mesg.find("Tput".encode("ascii"), 0)
                if (start > 0):
                    print "found throughput information"
                    break
                if (cn == (detect_loop - 1)):
                    print "no throughput message found"
                    sys.exit()
            except:
                pass
                #print "RF driver message skipped"
                
        # clear buffer
        for _ in range(10):
            s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"), 0.5)
        
        type(self).enb.enb_cd_usr_bin()
            
        per = 100.0
        tput = 0.0
        
        if (type(self).cfg.test_set == 'rs'):
            type(self).cmw500.send_msg_to_server("ROUTe:GPRF:GENerator:SCENario:SALone RFAC, TX1")
            
        s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"), 0.5)
        
        # test limit
        self.per_pass = False
        self.curr_pow = self.init_pow + type(self).cfg.rssi_cable_loss
        print("\nVSG output power is " + str(self.curr_pow) + ' dBm')
        
        if (type(self).cfg.test_set == 'agilent'):
            type(self).exg.send_msg_to_server(':POW ' + str(self.curr_pow) + ' dBm')
            self.exg_sync_trigger()
        elif (type(self).cfg.test_set == 'anritsu'):
            self.mt8870a_setup(self.curr_pow)
        elif (type(self).cfg.test_set == 'rs'):
            self.cmw500_setup(self.curr_pow)
        
        sleep(3)    # wait RF power stable
        #self.clear_telnet_message()
        
        self.isFirstGetPer = True
        print '\n*** Start receiving, press CTRL-C to stop. ***\n'
        
        try:
            while True:
                per, tput = self.get_per_tput()
                #per, tput = self.getPerFromLogParser()
                
                if tput >= 0:
                    print 'PER ' + str(per) + '% Tput ' + str(tput)
                    print ''
                
        except KeyboardInterrupt:
            print 'End of continue receive test'
        
            # instrument turn off
            if (type(self).cfg.test_set == 'agilent'):
                self.exg_turn_off()
            elif (type(self).cfg.test_set == 'anritsu'):
                self.mt8870a_turn_off()
            elif (type(self).cfg.test_set == 'rs'):
                self.cmw500_turn_off()
            

    def clear_telnet_message(self):
        
        for _ in range(8):
            s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"), 0.1)

    def getPerFromLogParser(self):
        
        self.r_per = 100.0
        self.pass_packets = -1
        start = 0
        
        if (self.isFirstGetPer == True):
            self.isFirstGetPer = False
            for _ in range(20): # get rid unwanted messages
                self.mesg = self.udpsvr.recv_frm_client()
                #print "log1st :" + self.mesg
                #self.rpt.write('log1st :' + self.mesg + '\n')
        else:
            for _ in range(10): # get rid unwanted messages
                self.mesg = self.udpsvr.recv_frm_client()
                #print "log2nd :" + self.mesg
                #self.rpt.write('log2nd :' + self.mesg + '\n')
        
        test_loop = 20
        for count in range(test_loop):
            
            self.mesg = self.udpsvr.recv_frm_client()
            start = self.mesg.find("PER:".encode("ascii"), start)
            #print "log777 :" + self.mesg
            #self.rpt.write('log777 :' + self.mesg + '\n')
            
            if (start == -1):
                if (count < (test_loop-1)):
                    start = 0
                    self.mesg = ""
                    sleep(2)
                    continue
                else:
                    print "log message failed"
                    return (self.r_per, self.pass_packets)
            else: break
            
        start += len("PER:")
        self.per = self.mesg[start:self.mesg.find("Tput:".encode("ascii"), start)]
        start = self.mesg.find("Tput:".encode("ascii"), start)
        start += len("Tput:")
        self.tput = self.mesg[start:self.mesg.find("kbps".encode("ascii"), start)]
        try:
            self.r_per = round(float(self.per.strip())*0.01, 2)
            self.pass_packets = int(self.tput.strip())
        except:
            print "per convert error: " + self.mesg
            print "start=" + str(start)
            
        #self.rpt.write('per ' + str(self.r_per) + ', passed ' + str(self.pass_packets) + '\n')
        return (self.r_per, self.pass_packets)

    def get_per_tput(self):
        # read form PHY register
        
        self.total_packets = 0.0
        self.pass_packets = 0.0
        self.r_per = 100.0
        
        # put this line before call function
        type(self).enb.tn_write(type(self).enb.pp_usrbin, "export OCTEON_REMOTE_PROTOCOL=linux")
        
        # clear the statistics
        type(self).enb.tn.write("/usr/bin/oct-linux-memory -w 4 -c 1 0x10f0000a2b7d0 0\n".encode("ascii"))

        #s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"))
        sleep(0.5) # balance packet delay
        type(self).enb.tn.write("/usr/bin/oct-linux-memory -w 4 -c 1 0x10f0000a2b800 0\n".encode("ascii"))
        
        self.clear_telnet_message()
        sleep(0.5) # wait for packets
        
        # get tested packets
        type(self).enb.tn.write("/usr/bin/oct-linux-memory -w 4 -c 1 0x10f0000a2b7d0\n".encode("ascii"))

        sleep(0.5)
        s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"))
        s1 = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"))
        #print s1
        
        type(self).enb.tn.write("/usr/bin/oct-linux-memory -w 4 -c 1 0x10f0000a2b800\n".encode("ascii"))

        sleep(0.5)
        s = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"))
        s2 = type(self).enb.tn.read_until((type(self).enb.pp_usrbin).encode("ascii"))
        #print s2
        
        start = 0
        while(True):
            start = s1.find("10f0000a2b7d0 : ".encode("ascii"), start)
            if(start == -1): break
            start += len("10f0000a2b7d0 : ")
            total_packets_str = s1[start:start+8]
            self.total_packets = int(str(total_packets_str), 16)
            #print("Total subframes tested: " + str(self.total_packets))
        
        start = 0
        while(True):
            start = s2.find("10f0000a2b800 : ".encode("ascii"), start)
            if(start == -1): break
            start += len("10f0000a2b800 : ")
            pass_packets_str = s2[start:start+8]
            self.pass_packets = int(str(pass_packets_str),16)
            #print("Number of subframes with CRC pass: " + str(self.pass_packets))
        
        #print("Tested subframes: " + str(self.total_packets))
        
        if (self.total_packets > 0) and (self.pass_packets > 0):
            if (self.pass_packets >= self.total_packets):
                self.r_per = 0.0
            else:
                self.r_per = round(100.0*(1.0 - 
                        float(self.pass_packets)/float(self.total_packets)), 2)
            
        return (self.r_per, self.pass_packets)

