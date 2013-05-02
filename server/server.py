#!/usr/bin/env python

# 
# File: server.py
# Get kernel logs from clients, and apply analysis methods
# Author: Shuang Liang <shuang.liang2012@temple.edu>
#

#******************************** Modules **************************************
from socket import *
import struct
from collections import namedtuple
import smspdu

KlogType = ['WRITE', 'READ', 'OPEN', 'CLOSE']
KLOGSIZE = 284        # Size of C struct klog_entry
#******************************** Program Entry ********************************
def main():
    HOST = ''
    #HOST = '129.32.94.230'
    PORT = 8888
    ADDR = (HOST, PORT)

    tcpSerSock = socket(AF_INET, SOCK_STREAM)
    tcpSerSock.bind(ADDR)
    tcpSerSock.listen(5)
    print 'Server started at %s:%d, waiting for connection...' % (HOST, PORT)
    # Wait for new connection, blocking
    tcpCliSock, addr = tcpSerSock.accept()
    print '...connected from ', addr 
    
    # Print kernel log
    while True:
        logbuf = tcpCliSock.recv(KLOGSIZE)
        e = getKlogEntry(logbuf)
        if not e:
            print 'Sock recv error!'
            continue;
        else:
            printKlogEntry(e)
            pdu = probeSmsSubmit(e, tcpCliSock)
            if pdu is not None:
                print 'SMS_SUBMIT %s to %s' % (pdu.user_data, pdu.tp_address)

            print 80*'='

    tcpCliSock.close()
    tcpSerSock.close()

def getKlogEntry(logbuf):
    klog_fmt = "iiiiiii256s"
    KlogEntry = namedtuple('KlogEntry', 'type hour min sec pid uid param_size param')

    if not logbuf or len(logbuf) != KLOGSIZE:
        return None
    e = KlogEntry._make(struct.unpack(klog_fmt, logbuf))
    return e

def printKlogEntry(klog):
    print 'time: %d:%d:%d' % (klog.hour, klog.min, klog.sec)
    print 'pid: %d, uid: %d' % (klog.pid, klog.uid)
    print 'type: ', KlogType[klog.type]
    print 'param(%d): %s' % (klog.param_size, klog.param)
   
def probeSmsSubmit(klog, sock):
    if klog.uid != 1001 or KlogType[klog.type] != "WRITE" or klog.param_size < 8:
        return None
    if klog.param.startswith("AT+CMGS="):
        # Swallow <CR>
        e = getKlogEntry(sock.recv(KLOGSIZE))
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # SMSC.addr + TPDU
        e = getKlogEntry(sock.recv(KLOGSIZE)) 
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        smsc_al = int(e.param[0]*10) + int(e.param[1])
        if smsc_al == 0:
            pdu =  smspdu.SMS_SUBMIT.fromPDU(e.param[2:e.param_size], 'sender')
        else:
            pdu = smspdu.SMS_SUBMIT.fromPDU(e.param[(4+smsc_al*2):e.param_size])
        return pdu

if __name__ == "__main__":
    main()
