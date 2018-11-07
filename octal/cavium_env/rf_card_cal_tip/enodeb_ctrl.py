#!/usr/bin/env python

import re
import socket
import os.path
import test_config
from time import sleep
from telnetlib import Telnet

from tempfile import mkstemp
from shutil import move
from os import remove, close

class enodeB_Ctrl():
    
    def __init__(self):
        
        self.cfg = test_config.EnbConfig()
        self.tn = Telnet()
        self.rfif_hab_driver = "oct-linux-memory"
        self.cfg.board_typ = "zen_ad"
        self.rfic = " adi"
        self.edit_ubootenv = "edit_ubootenv.sh"
        self.uboot_file = "/mnt/app/ubootenv"
        
        self.pp_base = "/usr/bin"
        self.pp_home = "~"
        self.pp_root = "~"
        self.pp_etc = "/etc"
        self.pp_tmpfs = "/tmpfs"
        self.pp_usrbin = "/usr/bin"
        self.pp_lsmsdc1 = "LSMSDC1"

    def start_telnet_session(self):
        res = 0
        print ""
        print(self.cfg.enb_ipaddr + " start telnet session")
        
        try:
            self.tn.open(self.cfg.enb_ipaddr, self.cfg.enb_tn_port, 10)
            self.telnet_response(self.cfg.enb_ipaddr + " telnet start")
        except socket.error, err:
            print "telnet connection not ready"
            res = -1
        finally:
            return res
        
    def telnet_response(self, message, timeout = 3):
        
        if (timeout == 0):
            result = self.tn.read_until(self.pp_home.encode("ascii"))
            if (result):
                print (message + " ok")
            else:
                print (message + " fail")
        else:
            result = self.tn.read_until("# ".encode("ascii"), timeout)
            if (result):
                print (message + " ok")
            else:
                print (message + " fail")
                
        return result

    def end_telnet_session(self):
        
        try:
            self.tn.close()
            print(self.cfg.enb_ipaddr + " close telnet session")
            return True
        except:
            print(self.cfg.enb_ipaddr + " close telnet error!")
        else:
            return False

    def get_macaddr(self):
        
        mac_addr = ""
        ip_addr = ""
        start = 0
        
        self.tn.write("ifconfig\n".encode("ascii"))
        s = self.tn.read_until((self.pp_home).encode("ascii"))
        
        while(True):
            start = s.find("inet addr:".encode("ascii"), start)
            if(start == -1): break

            start += len("inet addr:")
            ip_addr = s[start:s.find(" ".encode("ascii"), start)]
            print("ip_addr: " + ip_addr)
            
            if (ip_addr == (self.cfg.enb_ipaddr).encode("ascii")):
                start = s.rfind("HWaddr ".encode("ascii"), 0, start)
                start += len("HWaddr ")
                mac_addr = s[start:s.find(" ".encode("ascii"), start)]
                print("mac_addr: " + mac_addr)
                break

        return mac_addr
    
    def tn_write(self, prompt, message, timeout=None):
        
        #timeout = 3 #3 #improve enb.enb_eeprom_get_record_num() to speed up
        if (timeout == None):
            timeout = 1
        
        self.tn.write((message + "\n").encode("ascii"))
        try:
            res = self.tn.read_until((prompt).encode("ascii"), timeout)
            return res
        except:
            print "telnet response failed"
        
    def enb_check_connection(self):
        
        enbok = 0
        start = 0
        self.tn.write("ls /etc")
        res = self.tn.read_until("#".encode("ascii"), 3)
        start = res.find("usr".encode("ascii"), start)
        if (start == -1):
            print 'telnet connection failed'
            enbok = -1
        else:
            print 'telnet connection ok'
            
        return enbok
        
    def enb_cd_usr_bin(self):
        self.tn_write(self.pp_usrbin, "cd /usr/bin")

    def enb_cd_tmpfs(self):
        self.tn_write(self.pp_tmpfs, "cd /tmpfs")

    def enb_cd_etc(self):
        self.tn_write(self.pp_etc, "cd /etc")

    # in CLI
    def enb_reboot_in_dsp_run(self):
        
        self.tn.write('reboot\n')
        
    # in CLI
    def enb_reboot_in_dsp_run2(self):
        
        self.tn.write("exit\n")
        self.tn.write('reboot\n')
        
    def enb_login(self):
        
        try:
            self.tn.read_until("LSM login:".encode("ascii"), 3)
            self.tn_write("Password:", "root")
            self.tn_write(self.pp_home, self.cfg.login_pwd)
            print(self.cfg.enb_ipaddr + " enodeB login ok")
            return True
        except:
            return False

    def enb_cleanup_tmpfs_partition(self):
        
        self.tn_write(self.pp_root, "cd /")
        self.tn_write(self.pp_root, "rm -rf /tmpfs/*")
        self.tn_write(self.pp_root, "umount -l /tmpfs")
        self.tn_write(self.pp_root, "mount -t tmpfs -o size=256M /dev/shm /tmpfs")
        self.tn_write(self.pp_tmpfs, "cd /tmpfs")
        
    # in /usr/bin
    def enb_run_lsmLogDisp(self):
        
        binName = "/usr/bin/lsmLogDisp_pltD "     # lsmLogDisp
        #self.tn_write(self.pp_usrbin, "tftp -g -r " + binName + self.cfg.tftp_server_ip)
        #self.tn_write(self.pp_usrbin, "chmod +x " + binName)
        self.tn_write(self.pp_usrbin, "oncpu 0 " + binName + " -p " +
                      str(self.cfg.my_udp_port) + " -a " + self.cfg.my_udp_ipaddr + " &")
    
    # in root home
    def enb_set_1pps(self, rfdir):
        
        if (rfdir == 'tx'):
            clkcomp = '0x208d555555'
        else:
            if (self.cfg.cal_bandwidth == 20):
                clkcomp = '0x1046AAAAAA'
            elif (self.cfg.cal_bandwidth == 15):
                clkcomp = '0x1046AAAAAA' #'0x208d555555'
            elif (self.cfg.cal_bandwidth == 10):
                clkcomp = '0x208d555555'
            elif (self.cfg.cal_bandwidth == 5):
                clkcomp = '0x411AAAAAAA'
            else:
                clkcomp = '0x208d555555'
        
        self.tn_write(self.pp_home, "oct-linux-csr MIO_PTP_CLOCK_CFG 0x540003763")
        self.tn_write(self.pp_home, "oct-linux-csr MIO_PTP_PPS_THRESH_LO 0")
        self.tn_write(self.pp_home, "oct-linux-csr MIO_PTP_PPS_THRESH_HI 0")
        self.tn_write(self.pp_home, "oct-linux-csr MIO_PTP_PPS_LO_INCR 0x1dcd650000000000")
        self.tn_write(self.pp_home, "oct-linux-csr MIO_PTP_PPS_HI_INCR 0x1dcd650000000000")
        self.tn_write(self.pp_home, "oct-linux-csr MIO_PTP_CLOCK_COMP " + clkcomp)
        self.tn_write(self.pp_home, "oct-linux-csr MIO_PTP_PHY_1PPS_IN 0x18")
        self.tn_write(self.pp_home, "oct-linux-csr GPIO_BIT_CFG5 0x1")

    # in root home
    def enb_reset_rfic(self):
        
        self.tn_write(self.pp_home, "/usr/bin/oct-linux-csr GPIO_XBIT_CFG17 0x0")
        sleep(0.5)
        self.tn_write(self.pp_home, "/usr/bin/oct-linux-csr GPIO_XBIT_CFG17 0x1")

    # in /tmpfs
    def enb_load_file_in_tmpfs(self, file_name):
        
        self.tn_write(self.pp_tmpfs, "tftp -r " + file_name + " -g " +
                       self.cfg.tftp_server_ip)
        self.tn_write(self.pp_tmpfs, "chmod +x " + file_name)

    # in /
    def enb_load_lfmsoft(self):
        
        self.tn_write(self.pp_root, "cd /")
        #self.tn_write(self.pp_root, "tftp -r LFMSOFT_OCT_D.tgz -g " +
        #               self.cfg.tftp_server_ip)
        self.tn_write(self.pp_root, "tar -xvzf LFMSOFT_OCT_D.tgz")
        #self.tn_write(self.pp_root, "export LD_LIBRARY_PATH=/lib/")

    # in /usr/bin
    def enb_set_rfif_hab(self, rfdir):
        
        self.tn.write("export OCTEON_REMOTE_PROTOCOL=linux\n")
        # RF initialize form DDR. Write 0x0xEABE1234 to take the value from DDR, 
        # write 0 for disabling this option: default 10MHz will be configured for
        self.remote_memory_32bit_set(100, "0x83000000", "0xeabe1234")
        # RF Board Type
        self.remote_memory_32bit_set(100, "0x83000004", "0x1")
        # Rx Offset
        self.remote_memory_32bit_set(100, "0x83000008", "0x4")
        # Rx Lead
        self.remote_memory_32bit_set(100, "0x8300000c", "0x64")
        # Rx Lag
        self.remote_memory_32bit_set(100, "0x83000010", "0x0")
        
        # Tx Offset
        if (rfdir == 'tx'):
            self.remote_memory_32bit_set(100, "0x83000014", "0x648")
        else:
            if (self.cfg.cal_bandwidth == 20):
                self.remote_memory_32bit_set(100, "0x83000014", "0x288")
            elif (self.cfg.cal_bandwidth == 15):
                self.remote_memory_32bit_set(100, "0x83000014", "0x288")
            elif (self.cfg.cal_bandwidth == 10):
                self.remote_memory_32bit_set(100, "0x83000014", "0x7784")
            elif (self.cfg.cal_bandwidth == 5):
                self.remote_memory_32bit_set(100, "0x83000014", "0x3BC4")
            else:
                self.remote_memory_32bit_set(100, "0x83000014", "0x7784")
                
        # Tx Lead
        self.remote_memory_32bit_set(100, "0x83000018", "0x8c")
        # Tx Lag
        self.remote_memory_32bit_set(100, "0x8300001c", "0x3c")
        # 1:MIMO 0:SISO
        self.remote_memory_32bit_set(100, "0x83000020", "0x1")
        # 1:Dual Port 0:single port Should not be changed
        self.remote_memory_32bit_set(100, "0x83000024", "0x1")
        # SPI  not used now
        self.remote_memory_32bit_set(100, "0x83000028", "0x0")
        # SPI Device 0 : not used now
        self.remote_memory_32bit_set(100, "0x8300002c", "0x0")
        # 0:FDD 1:TDD 
        self.remote_memory_32bit_set(100, "0x83000030", "0x0")
        
        # Bandwidth 0:20MHz 1:15MHz 2:10Mhz 3:5MHz
        if (self.cfg.cal_bandwidth == 5):
            self.remote_memory_32bit_set(100, "0x83000034", "0x3")
        elif (self.cfg.cal_bandwidth == 10):
            self.remote_memory_32bit_set(100, "0x83000034", "0x2")
        elif (self.cfg.cal_bandwidth == 15):
            self.remote_memory_32bit_set(100, "0x83000034", "0x0") #0x01
        elif (self.cfg.cal_bandwidth == 20):
            self.remote_memory_32bit_set(100, "0x83000034", "0x0")
        else:
            self.remote_memory_32bit_set(100, "0x83000034", "0x2")
            
        # MAX sample Adjustment Should not be changed
        self.remote_memory_32bit_set(100, "0x83000038", "0x2")
        # MIN sample Adjustment Should not be changed
        self.remote_memory_32bit_set(100, "0x8300003c", "0x2")
        # Sample Add and drop at end of frame Should not be changed
        self.remote_memory_32bit_set(100, "0x83000040", "0x0")
        
        # sample at which rx sample counter adjustment Should not be changed
        if (self.cfg.cal_bandwidth == 20):
            self.remote_memory_32bit_set(100, "0x83000044", "0x7800")
        elif (self.cfg.cal_bandwidth == 15):
            self.remote_memory_32bit_set(100, "0x83000044", "0x7800") #"0x3C00"
        elif (self.cfg.cal_bandwidth == 10):
            self.remote_memory_32bit_set(100, "0x83000044", "0x3C00")
        elif (self.cfg.cal_bandwidth == 5):
            self.remote_memory_32bit_set(100, "0x83000044", "0x1E00")
        else:
            self.remote_memory_32bit_set(100, "0x83000044", "0x3C00")
            
        # sample at which tx sample counter adjustment Should not be changed
        if (self.cfg.cal_bandwidth == 20):
            self.remote_memory_32bit_set(100, "0x83000048", "0x7800")
        elif (self.cfg.cal_bandwidth == 15):
            self.remote_memory_32bit_set(100, "0x83000048", "0x7800") #"0x3C00"
        elif (self.cfg.cal_bandwidth == 10):
            self.remote_memory_32bit_set(100, "0x83000048", "0x3C00")
        elif (self.cfg.cal_bandwidth == 5):
            self.remote_memory_32bit_set(100, "0x83000048", "0x1E00")
        else:
            self.remote_memory_32bit_set(100, "0x83000048", "0x3C00")
        
        # rx correct adjustment Should not be changed
        self.remote_memory_32bit_set(100, "0x8300004c", "0x5")
        # tx correct adjustment Should not be changed
        self.remote_memory_32bit_set(100, "0x83000050", "0x8")
        
    def remote_memory_32bit_set(self, val_c, address, value):
        # oct-linux-memory
        self.tn_write(self.pp_usrbin, "/usr/bin/" + self.rfif_hab_driver + 
                          " -w 4 -c " + str(val_c) + " " + address + " " + value)

    def enb_get_delay_sync_value(self):
        
        self.tn.write('/usr/bin/oct-linux-memory -w 4 -c 120 0x10f00009a2040')
        res = self.tn.read_until(self.pp_tmpfs, 3)
        print "********"
        print res
        print "********"
        
    # in /usr/bin
    def enb_load_rf_init(self, rfInitName):
        
        self.tn_write(self.pp_usrbin, "tftp -gr " + rfInitName \
                      + " " + self.cfg.tftp_server_ip)
        #if not os.path.isfile(rfInitName):
        #    return False
        
        if self.cfg.rf_drv_init != "rf_init.txt":
            self.tn_write(self.pp_usrbin, "cp " + rfInitName + "rf_init.txt")
                            
        self.tn_write(self.pp_usrbin, "chmod +r rf_init.txt")
        return True
        
    # in /usr/bin
    def enb_set_rf_drv_rf_card(self):
        
        self.tn_write(self.pp_usrbin, "cp " + self.cfg.rf_drv_init + " rf_init.txt")
        self.tn_write(self.pp_usrbin, "chmod +r rf_init.txt")
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "i d " + str(test_config.dl_freq) + " u " + 
                      str(test_config.ul_freq) + " a 1 " + str(self.cfg.attn1) + 
                      " a 2 " + str(self.cfg.attn2) + " g 1 " + 
                      str(self.cfg.gain1) + " g 2 " + 
                      str(self.cfg.gain2) + " q")
        self.tn_write(self.pp_base, "EOF")
        
        #self.enb_set_adi_register27()
        
    def rx_set_rf_drv_rf_card(self):
    
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "a 1 " + str(self.cfg.attn1) + 
                      " a 2 " + str(self.cfg.attn2) + " g 1 " + 
                      str(self.cfg.rx_gain) + " g 2 " + 
                      str(self.cfg.rx_gain) + " q")
        self.tn_write(self.pp_base, "EOF")
        sleep(3)
        """
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "w 6 8 gpio 8 q")
        self.tn_write(self.pp_base, "EOF")
        
        self.enb_set_adi_register27()
        """
        
    # in /usr/bin
    def enb_soft_reset(self):
    
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "rst q")
        self.tn_write(self.pp_base, "EOF")
    
    def enb_set_zen_tdd_rx(self):
    
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "w 27 0x40 q")
        self.tn_write(self.pp_base, "EOF")
        
    def enb_set_zen_tdd_tx(self):
    
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "w 27 0x90 q")
        self.tn_write(self.pp_base, "EOF")
        
    # in /usr/bin
    def enb_enable_all_TX(self):
    
        #print "enable all TX ports"
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        if (self.cfg.cal_bandwidth >= 15):
            self.tn_write(self.pp_base, "w 2 0xCE q")
        else:
            self.tn_write(self.pp_base, "w 2 0xDE q")
        self.tn_write(self.pp_base, "EOF")
        
    # in /usr/bin
    def enb_disable_all_TX(self):
    
        print "disable all TX ports"
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        if (self.cfg.cal_bandwidth >= 15):
            self.tn_write(self.pp_base, "w 2 0x0E q")
        else:
            self.tn_write(self.pp_base, "w 2 0x1E q")
            
        self.tn_write(self.pp_base, "EOF")
        
    # in /usr/bin
    def enb_disable_TX1(self):
    
        print "disable primary TX"
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        if (self.cfg.cal_bandwidth >= 15):
            self.tn_write(self.pp_base, "w 2 0x8E q")
        else:
            self.tn_write(self.pp_base, "w 2 0x9E q")
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_disable_TX2(self):
    
        print "disable second TX"
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        if (self.cfg.cal_bandwidth >= 15):
            self.tn_write(self.pp_base, "w 2 0x4E q")
        else:
            self.tn_write(self.pp_base, "w 2 0x5E q")
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_enable_all_RX(self):
    
        print "enable all RX ports"
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "w 3 0xDE q")
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_disable_all_RX(self):
    
        print "disable all RX ports"
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "w 3 0x1E q")
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_disable_RX1(self):
    
        print "disable primary RX"
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        
        self.tn_write(self.pp_base, "g 1 5 g 2 60 q")
        """
        if (self.cfg.cal_bandwidth >= 15) and (test_config.band > 32):
            self.tn_write(self.pp_base, "w 3 0x9E q")
        else:
            self.tn_write(self.pp_base, "g 1 5 g 2 60 q")
        """
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_disable_RX2(self):
    
        print "disable second RX"
        self.tn_write(self.pp_usrbin, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        
        self.tn_write(self.pp_usrbin, "g 1 60 g 2 5 q")
        """
        if (self.cfg.cal_bandwidth >= 15) and (test_config.band > 32):
            self.tn_write(self.pp_base, "w 3 0x5E q")
        else:
            self.tn_write(self.pp_usrbin, "g 1 60 g 2 5 q")
        """
        self.tn_write(self.pp_usrbin, "EOF")
    
    def enb_pwm_ctrl_call_cmd(self, cmd):
        
        self.tn_write(self.pp_usrbin, "oncpu 0 /usr/bin/" + 
                      self.cfg.rf_driver + self.rfic + " <<  EOF")
        self.tn_write(self.pp_usrbin, "pwm " + cmd)
        self.tn_write(self.pp_usrbin, "EOF")
    
    def enb_dax_ctrl_call_cmd(self, cmd):
   
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + 
                      self.cfg.rf_driver + self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "dax " + cmd)
        self.tn_write(self.pp_base, "EOF")
    
    def enb_rf_drv_call(self):
        
        if (self.cfg.cal_bandwidth >= 15):
            self.tn_write("Option:", "oncpu 0 /usr/bin/" + 
                          self.cfg.rf_driver + self.rfic + " 26")
        else:
            self.tn_write("Option:", "oncpu 0 /usr/bin/" + 
                          self.cfg.rf_driver + self.rfic + " 16")
        
    def enb_rf_drv_call_cmd(self, cmd):
    
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + 
                      self.cfg.rf_driver + self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, cmd)
        self.tn_write(self.pp_base, "EOF")

    # in /usr/bin
    def enb_load_rf_driver(self):
        
        self.tn_write(self.pp_base, "tftp -gr " + self.cfg.rf_driver + " " +
                      self.cfg.tftp_server_ip)  

    # in /usr/bin
    def enb_load_dac(self):
        
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "dac " + "`fprintenv dac_offset -r`" + " q")
        self.tn_write(self.pp_base, "EOF")     

    def enb_load_pwm(self):
        
        self.tn_write(self.pp_base, "oncpu 0 " + self.cfg.rf_driver + 
                      self.rfic + " << EOF")
        self.tn_write(self.pp_base, 
                      "pwm e h `fprintenv pwmreghigh -r` l `fprintenv pwmreglow -r`" + 
                      " q")
        self.tn_write(self.pp_base, "EOF")
        
    def enb_load_dax(self):
        
        self.tn_write(self.pp_base, "oncpu 0 " + self.cfg.rf_driver + self.rfic + 
                      " << EOF")
        self.tn_write(self.pp_base, "dax `fprintenv pwmreghigh -r`" + " q")
        self.tn_write(self.pp_base, "EOF")
        
    def enb_load_iq_offset(self):
        reg_en_offset = 0x9f
        reg_tx1_i = 0x92
        reg_tx1_q = 0x93
        reg_tx2_i = 0x94
        reg_tx2_q = 0x95

        self.enb_adi_write_reg(str(hex(reg_tx1_i)), "`fprintenv tx1_i_offset -r`")
        self.enb_adi_write_reg(str(hex(reg_tx1_q)), "`fprintenv tx1_q_offset -r`")
        self.enb_adi_write_reg(str(hex(reg_tx2_i)), "`fprintenv tx2_i_offset -r`")
        self.enb_adi_write_reg(str(hex(reg_tx2_q)), "`fprintenv tx2_q_offset -r`")
        self.enb_adi_write_reg(str(hex(reg_en_offset)), str(hex(0x0C)))
        
    # in /etc
    def enb_load_dsp_to_etc(self):
        self.tn_write(self.pp_etc, "tftp -g -r dsp.tgz " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_etc, "tar -zxvf dsp.tgz")
        
    # in /tmpfs
    def enb_load_dsp_to_tmpfs(self):
        self.tn_write(self.pp_tmpfs, "tftp -g -r dsp.tgz " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_tmpfs, "tar xzf dsp.tgz")
        
    # in /usr/bin
    # set flag for MAC to get triggered
    def enb_set_7ffeff00(self):
        #self.tn_write(self.pp_base, "ms 7ffeff00 6 -w")
        self.tn.write("/usr/bin/oct-linux-memory -w 4 -c 8 0x7ffeff00 6\n")
        
    # in /tmpfs
    def enb_etm_test_vector(self):
        
        #self.tn_write(self.pp_tmpfs, "rm -rf /tmpfs/*")
        #sleep(2)
        
        self.tn_write(self.pp_tmpfs, "tftp -g -r " + self.cfg.dl_etm_test_vector + 
                      " " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_tmpfs, "tar -zxvf " + self.cfg.dl_etm_test_vector)
        self.tn_write(self.pp_tmpfs, "rm " + self.cfg.dl_etm_test_vector)
        
    # in /tmpfs
    def enb_load_LSM_X_TV_0_wk21_00_ETM(self):
        
        print 'loading ' + self.cfg.dl_etm_test_vector
        self.tn_write(self.pp_tmpfs, "tftp -g -r " + self.cfg.dl_etm_test_vector + 
                      " " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_tmpfs, "tar -xvf " + self.cfg.dl_etm_test_vector)
        self.tn_write(self.pp_tmpfs, "rm " + self.cfg.dl_etm_test_vector)
        
    # in /tmpfs
    def enb_load_LSM_X_L1_0_SoC_11_wk37_00(self):
        
        self.tn_write(self.pp_tmpfs, "tftp -g -r LSM_X_L1_0_SoC_11_wk37_00.tgz " + 
                      str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_tmpfs, "tar -zxvf LSM_X_L1_0_SoC_11_wk37_00.tgz ")
        self.tn_write(self.pp_tmpfs, "rm LSM_X_L1_0_SoC_11_wk37_00.tgz ")
        
    # in /tmpfs
    def enb_load_ul_test_vector(self):
        
        self.tn_write(self.pp_tmpfs, "tftp -g -r TV.tgz " + 
                      str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_tmpfs, "tar -xvf TV.tgz ")
        self.tn_write(self.pp_tmpfs, "rm TV.tgz ")
        
    # in /usr/bin
    def enb_set_dsp_app_dl(self):
        
        #self.tn_write(self.pp_base, "gzip -d ./pltD.gz")
        self.tn_write(self.pp_tmpfs, "tftp -gr pltD-0619 " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_base, "mv ./pltD-0619 ./pltD")
        
        self.tn_write(self.pp_base, "chmod +x ./pltD")
        self.tn_write(self.pp_base, "oncpu 0 ./pltD -ld normal -stall 2")
        
    # in /usr/bin
    def enb_run_dsp_app_dl(self):
        
        """
        print "oncpu 0 /usr/bin/pltD -drv dl -bw " \
                      + str(self.cfg.cal_bandwidth) \
                      + " -tc 0001 -etm 1 -ant 2 -c 1 &"
        """
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/pltD -drv dl -bw " \
                      + str(self.cfg.cal_bandwidth) \
                      + " -tc 0001 -etm 1 -ant 2 -c 1 &")
        
    # in /tmpfs
    def enb_run_dsp_app_dl_15mhz(self):
        
        self.tn_write(self.pp_tmpfs, "oncpu 0 ./pltD -drv dl -bw 20 " \
                      + " -tc 0001 -etm 1 -ant 2 -c 1 &")
        
    # in /tmpfs
    def enb_stop_transmit(self):
        
        self.tn_write(self.pp_tmpfs, '\x1d')
        self.tn_write(self.pp_tmpfs, "oncpu 0 ./pltD -drv stop")
        
    # in /usr/bin
    def enb_adi_write_reg(self, addr, val):

        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "w " + str(addr) + " " + str(val) + " q")
        self.tn_write(self.pp_base, "EOF")
        
    # in /usr/bin
    def enb_adi_read_reg(self, addr, val):

        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "r " + str(addr) + " q")
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_eeprom_get_record_num(self):
        
        num_records = '-1'
        sleep(5)
        try:
            self.tn_write('Option', "oncpu 0 /usr/bin/" + \
                          self.cfg.rf_driver + self.rfic, 3)
            self.tn_write('option', "e", 3)
            self.tn_write(':', "rnr", 3)
            res = self.tn.read_until(')'.encode("ascii"), 5)
            #self.tn_write('/usr/bin>', "q")
            self.tn_write(self.pp_base, "q", 3)
            
            start = 0
            start = res.find("num_records = ".encode("ascii"), start)
            start += len("num_records = ")
            num_records = res[start:res.find('('.encode("ascii"), start)]
        except:
            print 'fail to get eeprom record number'
            
        return num_records
    
    # in /usr/bin
    def enb_eeprom_get_serial_num(self):
        
        self.tn_write('Option', "oncpu 0 /usr/bin/" + self.cfg.rf_driver + self.rfic)
        self.tn_write('option', "e")
        self.tn_write(':', "rsn")
        res = self.tn.read_until(')'.encode("ascii"), 15)
        self.tn_write('/usr/bin>', "q")
        
        start = 0
        start = res.find("serialno = ".encode("ascii"), start)
        start += len("serialno = ")
        serial_num = res[start:res.find('\n'.encode("ascii"), start)]
        return serial_num
    
    # in /usr/bin
    def enb_eeprom_get_earfcn_dl(self):
        
        self.tn_write('Option', "oncpu 0 /usr/bin/" + self.cfg.rf_driver + self.rfic)
        self.tn_write('option', "e")
        self.tn_write(':', "lre")
        res = self.tn.read_until('*'.encode("ascii"), 15)
        self.tn_write('/usr/bin>', "q")
        
        start = 0
        self.records = []
        
        while(True):
            start = res.find("record: ".encode("ascii"), start)
            if (start == -1): break
            start += len("record: ")
            rec_num = res[start:res.find('earfcn: '.encode("ascii"), start)]
            start = res.find("earfcn: ".encode("ascii"), start)
            start += len("earfcn: ")
            earfcn = res[start:res.find('\n'.encode("ascii"), start)]
            
            self.records.append([rec_num, earfcn])
            print "record: " + rec_num + " earfcn: " + earfcn
        
        return self.records
    
    # in /usr/bin
    def enb_bb_eeprom_get_serial_number(self):
        
        _ = self.tn.read_until('login'.encode("ascii"), 5)
        print ''
        self.tn_write('Option', "oncpu 0 /usr/bin/" + self.cfg.rf_driver)
        self.tn_write('option', "be")
        self.tn_write(':', "rsn")
        res = self.tn.read_until('*'.encode("ascii"), 5)
        self.tn_write('/usr/bin', "q")
        
        start = 0
        start = res.find("= ".encode("ascii"), start)
        start += len("= ")
        serialNum = res[start:res.find('*'.encode("ascii"), start)].strip()
        return serialNum
    
    # in /usr/bin
    def enb_eeprom_to_uboot(self):
        
        print "\nEERPOM content:"
        records = self.enb_eeprom_get_earfcn_dl()
        efn_num = int(raw_input("input record earfcn: "))
        
        self.got_earfcn_rec = False
        for rn in records:
            if (efn_num == int(rn[1])):
                self.got_earfcn_rec = True
                break
        
        if (self.got_earfcn_rec == False):
            print "no available record for earfcn " + str(efn_num)
            return
        
        self.tn_write('Option', "oncpu 0 /usr/bin/" + self.cfg.rf_driver + self.rfic)
        self.tn_write('option', "e")
        self.tn_write('number', "lod")
        self.tn_write(':', str(efn_num))
        self.tn_write('/usr/bin>', "q")
        
        print "writing uboot environment variables..."
        while (True):
            res = self.tn.read_until('='.encode("ascii"), 60)
            if (res.find("tx_max_pwr_sec".encode("ascii")) == -1):
                print "done"
                break
            else:
                continue
    
    # in /usr/bin
    def enb_eeprom_edit_header(self, cmd, val):
        
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "e")
        self.tn_write(self.pp_base, cmd + " " + val)
        self.tn_write(self.pp_base, "q")
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_eeprom_edit_rec_num(self, cmd, val):
        
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "e")
        self.tn_write(self.pp_base, cmd + " " + str(val))
        self.tn_write(self.pp_base, "q")
        self.tn_write(self.pp_base, "EOF")
    
    # in /usr/bin
    def enb_eeprom_edit_record(self, cmd, rec_num, val):
        
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "e")
        self.tn_write(self.pp_base, "wrc " + str(rec_num) + " " + cmd + " " + val)
        self.tn_write(self.pp_base, "q")
        self.tn_write(self.pp_base, "EOF")

    # in /usr/bin
    def enb_bb_eeprom_edit_record(self, cmd, val):
        
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "be")
        self.tn_write(self.pp_base, cmd + " " + val)
        self.tn_write(self.pp_base, "q")
        self.tn_write(self.pp_base, "EOF")

    # in /usr/bin
    def enb_eeprom_append_record(self, cmd, rec_ver, val):
        
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "e")
        self.tn_write(self.pp_base, "arc " + str(rec_ver) + " " + cmd + " " + val)
        self.tn_write(self.pp_base, "q")
        self.tn_write(self.pp_base, "EOF")

    # in /usr/bin
    def enb_get_temperature(self):
        
        self.tn_write('Option', "oncpu 0 /usr/bin/" + self.cfg.rf_driver + self.rfic)
        self.tn_write(':', "mr")
        res = self.tn.read_until(')'.encode("ascii"), 5)
        self.tn_write('/usr/bin>', "q")
        #print "res = " + res
        
        start = 0
        start = res.find("temperature= ".encode("ascii"), start)
        start += len("temperature= ")
        temperature = res[start:res.find('('.encode("ascii"), start)]
        return temperature
        
    # in /usr/bin
    def enb_set_adi_register27(self):
        
        self.reg27 = '0x80'
        self.tn_write(self.pp_base, "oncpu 0 /usr/bin/" + self.cfg.rf_driver + 
                      self.rfic + " <<  EOF")
        self.tn_write(self.pp_base, "w 27 " + self.reg27 + " q")
        self.tn_write(self.pp_base, "EOF")
        #print "ADI register 27 set to " + self.reg27
        
    # in /usr/bin
    def rx_load_test_vector(self):
        #self.tn_write(self.pp_tmpfs, "rm CAL_UL_TV_" + str(self.cfg.cal_bandwidth) + "Mhz.tgz")
        #self.tn_write(self.pp_tmpfs, "rm CAL_ETM_TV_" + str(self.cfg.cal_bandwidth) + "Mhz.tgz")
        #self.tn_write(self.pp_tmpfs, "rm -rf TV")
        #sleep(1)
        testVector = ''
        if (self.cfg.cal_bandwidth == 5):
            testVector = 'CAL_UL_TV_5Mhz.tgz'
        elif (self.cfg.cal_bandwidth == 10):
            testVector = 'CAL_UL_TV_10Mhz.tgz'
        elif (self.cfg.cal_bandwidth == 20):
            testVector = 'CAL_UL_TV_20Mhz.tgz'
        
        self.tn_write(self.pp_base, "tftp -g -r " + testVector + " " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_base, "tar xzvf " + testVector)
        self.tn_write(self.pp_base, "rm " + testVector)
    
    """
    # in /tmpfs
    def rx_load_test_vector(self):
        
        self.tn_write(self.pp_tmpfs, "tftp -g -r " + self.cfg.rx_test_vector + 
                      " " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_tmpfs, "tar xzf " + self.cfg.rx_test_vector)
        self.tn_write(self.pp_tmpfs, "rm " + self.cfg.rx_test_vector)
    """
    
    # in /tmpfs
    def rx_load_patch_vector(self):
        
        self.tn.write(("cd /tmpfs/TV/UL/" + 
                       str(self.cfg.cal_bandwidth) + "m").encode("ascii"))
        self.tn_write(self.pp_tmpfs, "tftp -g -r " + self.cfg.rx_patch_vector + 
                      " " + str(self.cfg.tftp_server_ip))
        self.tn_write(self.pp_tmpfs, "tar xzf " + self.cfg.rx_patch_vector)
        self.tn_write(self.pp_tmpfs, "rm " + self.cfg.rx_patch_vector)

    # in /tmpfs
    def rx_run_dsp_app(self):

        self.tn_write(self.pp_tmpfs, "oncpu 0 /usr/bin/pltD -drv ul -bw " +
                        str(self.cfg.cal_bandwidth) + " -tc " + 
                        self.cfg.tcid + " -c 1 &")

    # in CLI
    def rx_display_bler(self):
        
        self.tn.write("ccli << EOF\n")
        self.tn.write("drvCmd -u mask 800\n")
        self.tn.write("exit\n")
        self.tn.write("EOF\n")
    
    # in /usr/bin
    def get_delay_sync_value(self):
        
        self.tn.write("cd /usr/bin\n")
        self.tn.read_until(self.pp_usrbin, 5)
        self.tn.write("export OCTEON_REMOTE_PROTOCOL=linux\n")
        
        self.tn.write("/usr/bin/oct-linux-memory -w 4 -c 120 0x10f00009a2040\n")
        self.tn.read_until('10f00009a2060'.encode("ascii"), 5)
        sleep(5)
        
        self.tn.write("/usr/bin/oct-linux-memory -w 4 -c 120 0x10f00009673ec 4096\n")
        self.tn.read_until('10f0000a2b7f0'.encode("ascii"), 5)
        sleep(5)
        
        self.tn.write("/usr/bin/oct-linux-memory -w 4 -c 120 0x10f00009a2040\n")
        res = self.tn.read_until('10f00009a2060'.encode("ascii"), 5)
        #print res
        start = res.find("10f00009a2040 : ".encode("ascii"), 0)
        start += len("10f00009a2040 : ")
        sync_val = res[start:res.find(" ".encode("ascii"), start)]
        #print 'sync_val = ' + sync_val
        
        if (sync_val != ""):
            print 'sync_val = ' + sync_val
            sync_val = '0x' + sync_val
            
            try:
                tohex = int(sync_val, 16) 
                tohex = 0 #TODO: fix
            except:
                tohex = 0
            
            if (tohex < 255) and (tohex >= 0):
                print 'tohex = ' + str(tohex)
            else:
                tohex = -1    
        else:
            tohex = 0
            
        self.tn.write("/usr/bin/oct-linux-memory -w 4 -c 120 0x10f0000a2b7d0\n")
        sleep(2)
        self.tn.write("/usr/bin/oct-linux-memory -w 4 -c 120 0x10f0000a2b7d0\n")
            
        return tohex
        
    # in /usr/bin
    def calc_rf_eeprom_md5(self):
        
        self.tn_write(self.pp_base, \
            '/usr/bin/get_eeprom_config.sh /tmp/rf.conf -rf -updmd5', 3)
        _ = self.tn.read_until('get_eeprom_config'.encode("ascii"), 3)
        res = self.tn.read_until('#'.encode("ascii"), 3)
        #print res
        start = res.find("writing".encode("ascii"), 0)
        if (start == -1): return 'non'
        start += len("writing: ")
        val_md5 = res[start:res.find('\n'.encode("ascii"), start)]
        #print 'MD5 = ' + str(val_md5)
        return str(val_md5)
        
    # in /usr/bin
    def wr_ubootenv_bw(self, bw):
        """
        cmd1 = 'sh /mnt/app/' + self.edit_ubootenv + ' BW ' + str(bw)
        self.tn_write(self.pp_base, 'tftp -gr ' + self.edit_ubootenv + ' ' \
                      + self.cfg.tftp_server_ip, 3)
        self.tn_write(self.pp_base, cmd1)
        """
        self.editUbootenv('BW', str(bw))
        
    # in /usr/bin
    def wr_ubootenv_freq(self, dl_freq, ul_freq):
        """
        cmd1 = 'sh /mnt/app/' + self.edit_ubootenv + ' TXFREQ ' + str(dl_freq)
        cmd2 = 'sh /mnt/app/' + self.edit_ubootenv + ' RXFREQ ' + str(ul_freq)
        
        self.tn_write(self.pp_base, 'tftp -gr ' + self.edit_ubootenv + ' ' \
                      + self.cfg.tftp_server_ip, 3)
        self.tn_write(self.pp_base, 'chmod +x ' + self.edit_ubootenv)
        self.tn_write(self.pp_base, cmd1)
        self.tn_write(self.pp_base, cmd2)
        """
        self.editUbootenv('TXFREQ', str(dl_freq))
        self.editUbootenv('RXFREQ', str(ul_freq))
        
    # in /usr/bin
    def wr_ubootenv_gain(self, gain1, gain2):
        """
        cmd1 = 'sh /mnt/app/' + self.edit_ubootenv + ' GAIN1 ' + str(gain1)
        cmd2 = 'sh /mnt/app/' + self.edit_ubootenv + ' GAIN2 ' + str(gain2)
        
        self.tn_write(self.pp_base, 'tftp -gr ' + self.edit_ubootenv + ' ' \
                      + self.cfg.tftp_server_ip, 3)
        self.tn_write(self.pp_base, 'chmod +x ' + self.edit_ubootenv)
        self.tn_write(self.pp_base, cmd1)
        self.tn_write(self.pp_base, cmd2)
        """
        self.editUbootenv('GAIN1', str(gain1))
        self.editUbootenv('GAIN2', str(gain2))
        
    # in /usr/bin
    def wr_ubootenv_atten(self, attn1, attn2):
        """
        cmd1 = 'sh /mnt/app/' + self.edit_ubootenv + ' ATTEN1 ' + str(attn1)
        cmd2 = 'sh /mnt/app/' + self.edit_ubootenv + ' ATTEN2 ' + str(attn2)
        
        self.tn_write(self.pp_base, 'tftp -gr ' + self.edit_ubootenv + ' ' \
                      + self.cfg.tftp_server_ip, 3)
        self.tn_write(self.pp_base, 'chmod +x ' + self.edit_ubootenv)
        self.tn_write(self.pp_base, cmd1)
        self.tn_write(self.pp_base, cmd2)
        """
        self.editUbootenv('ATTEN1', str(attn1))
        self.editUbootenv('ATTEN2', str(attn2))
        
    def editUbootenv(self, param, newval):
        
        cmd = 'sed -i \"s/^' + param \
                + '.*/' + param + '=\\\"' + newval \
                + '\\\"/\" /mnt/app/ubootenv'
        
        self.tn_write(self.pp_base, cmd.encode('ascii'))
    
    def killPltD(self):
        
        cmd = 'ps -e | grep pltD'
        stdin, stdout, stderr = self.sh.exec_command(cmd)
        gr = stdout.read()
        print gr
        print ""
        
        self.rlist = []
        if (len(gr) > 0) and (gr[0] != ' '):
            self.rlst = re.findall(r'^(.*)\s\?', gr, re.M|re.I)
        else:
            self.rlst = re.findall(r'^\s+(.*)\s\?', gr, re.M|re.I)
            
        for rt in self.rlst:
            pid = rt.split(' ')
            if pid[0] != '':
                print 'kill pltD, pid=' + pid[0]
                stdin, stdout, stderr = self.sh.exec_command('kill ' + pid[0])
                stdin, stdout, stderr = self.sh.exec_command('kill ' + pid[0])
        
        if (len(gr) > 0) and (gr[0] != ' '):
            self.rlst = re.findall(r'^(.*)\spts', gr, re.M|re.I)
        else:
            self.plst = re.findall(r'^\s+(.*)\spts', gr, re.M|re.I)
        
        for rt in self.rlst:
            pid = rt.split(' ')
            if pid[0] != '':
                print 'kill iperf, pid=' + pid[0]
                stdin, stdout, stderr = self.sh.exec_command('kill ' + pid[0])
                stdin, stdout, stderr = self.sh.exec_command('kill ' + pid[0])
        
    
    """
    def checkUbootenvFile(self):
        
        self.tn_write(self.pp_base, 'cd /usr/bin')
        self.tn_write(self.pp_base, 'ls /mnt/app/* | grep ubootenv')
        res = self.tn.read_until('#'.encode("ascii"), 3)
        #print res
        start = res.find("/mnt/app/ubootenv".encode("ascii"), 0)
        
        if (start >= 0):
            print 'found uboot env file'
        else:
            print 'create initial uboot env file'
            self.tn_write(self.pp_base, 'touch ' + self.uboot_file)
            self.tn_write(self.pp_base, 'chmod 777 ' + self.uboot_file)
            self.setUbootenvParameters()
            self.tn_write(self.pp_base, 'fsetenv mk_ubootenv 1', 3)
            
    def setUbootenvParameters(self):
        
        self.tn_write(self.pp_base, 'touch ' + self.uboot_file)
        self.wrUbootenvLine('ATTEN1', str(self.cfg.attn1))
        self.wrUbootenvLine('ATTEN2', str(self.cfg.attn2))
        self.wrUbootenvLine('BW', str(self.cfg.cal_bandwidth))
        self.wrUbootenvLine('EARFCN', '0')
        self.wrUbootenvLine('GAIN1', str(self.cfg.gain1))
        self.wrUbootenvLine('GAIN2', str(self.cfg.gain2))
        self.wrUbootenvLine('LOGDISPEN', '2')
        self.wrUbootenvLine('LOGDISPPORT', str(self.cfg.my_udp_port))
        self.wrUbootenvLine('LOGPARSERIP', str(self.cfg.my_udp_ipaddr))
        self.wrUbootenvLine('MODE', 'pltd')
        self.wrUbootenvLine('OVRDRFDSP', '1')
        self.wrUbootenvLine('PWMREGHIGH', '23456')
        self.wrUbootenvLine('PWMREGLOW', '37376')
        self.wrUbootenvLine('RFTARGET', 'zen')
        self.wrUbootenvLine('RSSI_OFFSET_PRIM', '0')
        self.wrUbootenvLine('RSSI_OFFSET_SEC', '0')
        self.wrUbootenvLine('RSSI_SLOPE_PRIM', '0')
        self.wrUbootenvLine('RSSI_SLOPE_SEC', '0')
        self.wrUbootenvLine('RXFREQ', str(test_config.ul_freq))
        self.wrUbootenvLine('SEBYPASS', '0')
        self.wrUbootenvLine('STARTAPP', '0')
        self.wrUbootenvLine('TCXO', 'pwm')
        self.wrUbootenvLine('TXFREQ', str(test_config.dl_freq))
        self.wrUbootenvLine('TX1_I_OFFSET', '0')
        self.wrUbootenvLine('TX1_Q_OFFSET', '0')
        self.wrUbootenvLine('TX2_I_OFFSET', '0')
        self.wrUbootenvLine('TX2_Q_OFFSET', '0')
        self.wrUbootenvLine('TX_OFFSET_PRIM', '0')
        self.wrUbootenvLine('TX_OFFSET_SEC', '0')
        self.wrUbootenvLine('TX_SLOPE_PRIM', '0')
        self.wrUbootenvLine('TX_SLOPE_SEC', '0')
        
    def wrUbootenvLine(self, param, value):
        
        self.tn_write(self.pp_base, 'echo \'' + param + '=\"' \
                      + value + '\"\' >> ' + self.uboot_file)
    """
    
def main():

    enb = enodeB_Ctrl()
    
    enb.start_telnet_session()
    enb.enb_login()
    enb.get_macaddr()
    enb.enb_set_1pps()
    
    enb.enb_cd_usr_bin()
    enb.enb_load_rf_drv()
    enb.enb_load_rf_init()
    
    #temp = enb.enb_get_temperature()
    #print "temp = " + temp
    
    enb.enb_set_rf_drv_big_rf_board()
    
    enb.enb_cd_etc()
    enb.enb_load_dsp_to_etc()
    
    enb.enb_cd_tmpfs()
    enb.enb_load_LSM_X_L1_0_July10()
    enb.enb_set_dsp_app_dl()
    enb.enb_set_7ffeff00()
    
    enb.enb_cleanup_tmpfs_partition()
    enb.enb_cd_tmpfs()
    enb.enb_load_LSM_X_TV_0_wk21_00_ETM()
    enb.enb_run_dsp_app_dl()
    sleep(3)
    
    enb.end_telnet_session()
    return 0
    
if (__name__ == "__main__"):
    main()
