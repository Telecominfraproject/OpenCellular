#!/usr/bin/env python

import sys
import socket
from time import sleep


class TcpClient():

    def __init__(self, host_ip, host_port):
        
        self.host_ip = host_ip
        self.host_port = host_port
        self.in_msg = ""
        self.out_msg = ""
        self.sock = None
        self.recv_buf_size = 2048
        
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.settimeout(10)
            print("tcp socket ok")
        except socket.error, msg:
            sys.stderr.write("[ERROR] %s\n" % msg[1])
            raise
        
    def tcp_connect(self):
        
        if (self.sock != None):
            try:
                self.sock.connect((self.host_ip, self.host_port))
                print("tcp connect ok")
            except socket.error, msg:
                sys.stderr.write("[ERROR] %s\n" % msg[1])
                raise
        else:
            print(self.host_ip + ": " + "skip tcp connect")
        
    def send_msg_to_server(self, message):
        self.out_msg = message
        nsend = self.sock.send(self.out_msg + '\n')
        if (nsend > 0):
            #print('sent '+ self.host_ip + '= ' + self.out_msg)
            pass
        
    def recv_msg_frm_server(self):
        self.in_msg = ""
        self.in_msg = self.sock.recv(self.recv_buf_size)
        
        return self.in_msg

    def tcp_close(self):
        self.sock.close()
        print(self.host_ip + " close tcp socket")
        
def main():
    # test exg
    HOST = '192.168.51.204'
    PORT = 5025
    
    MSG1 = '*IDN?'
    MSG2 = ':FREQ:CENT 2125 MHz'
    MSG3A = ':POW -'
    MSG3B = ' dBm'
    
    tcpc = TcpClient(HOST, PORT)
    tcpc.tcp_connect()
    sleep(1)
    tcpc.send_msg_to_server(MSG1)
    in_msg = tcpc.recv_msg_frm_server()
    print('recv ' + tcpc.host_ip + '= ' + in_msg)
    
    tcpc.send_msg_to_server(MSG2)
    
    for cnt in range(5):
        tcpc.send_msg_to_server(MSG3A + str(cnt*10 + 20) + MSG3B)
        sleep(2)
    
    tcpc.tcp_close()
    print("\nend of tcp_client")
    
    return 0
    
if (__name__ == "__main__"):
    main()
