#!/user/bin/env python

import sys
import socket

MSG_ACK = "UDPSRVACK"

class UdpServer():
    
    def __init__(self, my_ip, my_port):
        
        self.my_addr = my_ip
        self.my_port = my_port
        self.their_addr = ""
        self.their_port = ""
        self.tcp_addr = ""
        self.tcp_port = ""
        self.tcp_info = []
        self.msg = ""
        self.sock = ""
        
    def start_socket(self):
        
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            #self.host = self.sock.getsockname()
            self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.sock.settimeout(180)     # 5
            self.sock.bind((self.my_addr, self.my_port))
            print "start UDP server"
        except socket.error:
            errno, errstr = sys.exc_info()[:2]
            print("couldn't be a UDP server on port " + str(self.my_port) 
                  + str(errstr) + ' errno: ' + str(errno))
        
    def recv_frm_client(self):
        self.msg = ''
        self.msg, addr = self.sock.recvfrom(2048)
        #self.msg, _ = self.sock.recvfrom(20480)
        #print self.msg
        return self.msg
        
    def send_to_client(self):
        print("msg-> ["+MSG_ACK+"] to IP "+self.their_addr+" port "+str(self.their_port))
        self.sock.sendto(MSG_ACK, (self.their_addr, self.their_port))
        
        
    def udp_close(self):
        self.sock.close()

def main():

    udpsvr = UdpServer('10.18.104.189', 9991)
    udpsvr.start_socket()
    
    for i in range(10):
        i = i + 1
        mesg = udpsvr.recv_frm_client()
        print(mesg)
    
    udpsvr.udp_close()
    print "end of UDP server"
    
    return 0
            
if (__name__ == "__main__"):
    main()            
