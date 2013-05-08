#!/usr/bin/env python

# 
# File: server.py
# Get kernel logs from clients, and apply analysis methods
# Author: Shuang Liang <shuang.liang2012@temple.edu>
#
#--------------------------------------------------------------------
# Required Packages:
# smspdu > https://pypi.python.org/pypi/smspdu

#******************************** Modules *************************************#
from socket import *
import struct
from collections import namedtuple
import smspdu

#******************************** Definitions *********************************#
KlogType = ['WRITE', 'READ', 'OPEN', 'CLOSE', 'SOCKETCALL',
            'SOCKET', 'BIND', 'CONNECT', 'LISTEN', 'ACCEPT', 'SEND', 'SENDTO',
            'RECV', 'RECVFROM']

KLOGSIZE = 284        # Size of C struct klog_entry

#******************************** Program Entry *******************************#
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
    # Initialize event recording
    klogRecFile = open("klogs.txt", "w") 
    # Signature probing
    while True:
        #klogRecFile.write(80*'='+'\n')
        logbuf = tcpCliSock.recv(KLOGSIZE)
        e = rawStream2Klog(logbuf)
        if not e:
            print 'Sock recv error!'
            continue;
        else:
            writeToFile(klogRecFile, e)
            if e.param and e.param.startswith("AT"):
                # Ignore AT+CSQ
                if e.param.startswith("AT+CSQ"):
                    continue
                # Ignore AT+CLCC
                #if e.param.startswith("AT+CLCC"):
                    #continue
                print 80*'='
                printKlogEntry(e)
                print 80*'-'
                # Probe SMS-SUBMIT
                pdu = probeSmsSubmit(e, tcpCliSock)
                if pdu:
                    print 'Send SMS [%s] to %s' % (pdu.user_data, pdu.tp_address)
                    klogRecFile.write('Send SMS [%s] to %s\n' % (pdu.user_data, pdu.tp_address))
                    klogRecFile.write(80*'='+'\n')
                    continue
                # Probe SMS-DELIVER
                pdu = probeSmsReceive(e, tcpCliSock)
                if pdu:
                    print 'Receive SMS [%s] from %s' % (pdu.user_data, pdu.tp_address)
                    klogRecFile.write('Receive SMS [%s] from %s\n' % (pdu.user_data, pdu.tp_address))
                    klogRecFile.write(80*'='+'\n')
                    continue
                # Probe outgoing call
                dst_num = probeOutgoingCall(e)
                if dst_num:
                    print 'Calling %s' % (dst_num)
                    klogRecFile.write('Calling %s\n' % (dst_num))
                    klogRecFile.write(80*'='+'\n')
                    continue
                # Probe incoming call
                clcc_response = probeIncomingCall(e, tcpCliSock)
                if clcc_response:
                    print 'Incoming call %s' % (clcc_response)
                    klogRecFile.write('Incoming call %s\n' % (clcc_response))
                    klogRecFile.write(80*'='+'\n')
                    continue

    klogRecFile.close()
    tcpCliSock.close()
    tcpSerSock.close()

def rawStream2Klog(logbuf):
    klog_fmt = "iiiiiii256s"
    KlogEntry = namedtuple('KlogEntry', 'type hour min sec pid uid param_size param')

    if not logbuf or len(logbuf) != KLOGSIZE:
        return None
    e = KlogEntry._make(struct.unpack(klog_fmt, logbuf))

    if not  getKlogType(e):
        return None
    return e

def getKlogType(klog):
    if klog.type > len(KlogType)-1:
        return None
    else:
        return KlogType[klog.type]
    
def printKlogEntry(klog):
    print 'time: %d:%d:%d' % (klog.hour, klog.min, klog.sec)
    print 'pid: %d, uid: %d' % (klog.pid, klog.uid)
    print 'type: ', KlogType[klog.type]
    print 'param(%d): %s' % (klog.param_size, klog.param)

def writeToFile(fp, klog):
    fp.write('time: %d:%d:%d\n' % (klog.hour, klog.min, klog.sec)) 
    fp.write('pid: %d, uid: %d\n' % (klog.pid, klog.uid))
    fp.write('type: %s\n' % KlogType[klog.type])
    fp.write('param(%d): %s\n' % (klog.param_size, klog.param[:klog.param_size]))
    fp.write(80*'-'+'\n')

def probeSmsSubmit(klog, sock):
    if klog.uid != 1001 or KlogType[klog.type] != "WRITE":
        return None
    if klog.param.startswith("AT+CMGS="):
        # Swallow <CR>
        e = rawStream2Klog(sock.recv(KLOGSIZE))
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # SMSC.addr + TPDU
        while True:
            e = rawStream2Klog(sock.recv(KLOGSIZE)) 
            if not e:
                return None
            printKlogEntry(e)
            print 80*'-'
            if e.uid == 1001 and KlogType[e.type] == 'WRITE':
                break

        smsc_al = int(e.param[0])*10 + int(e.param[1])
        if smsc_al == 0:
            pdu =  smspdu.SMS_SUBMIT.fromPDU(e.param[2:e.param_size], 'sender')
        else:
            pdu = smspdu.SMS_SUBMIT.fromPDU(e.param[(4+smsc_al*2):e.param_size], 'sender')
        return pdu
    else:
        return None
def probeSmsReceive(klog, sock):
    if klog.uid != 1001 or not klog.param:
        return None
    if klog.param.startswith("AT+CNMA=1"):
        # Swallow CTL+M
        e = rawStream2Klog(sock.recv(KLOGSIZE))
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # SMS-DELIVER PDU
        e = rawStream2Klog(sock.recv(KLOGSIZE))
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # Decode PDU
        # Find OK
        subs = e.param.split('\n')
        #print subs
        smsc_al = int(subs[1][0])*10 + int(subs[1][1])
        if smsc_al == 0:
            pdu =  smspdu.SMS_DELIVER.fromPDU(subs[1][2:], 'receiver')
        else:
            pdu = smspdu.SMS_DELIVER.fromPDU(subs[1][(4+smsc_al*2):], 'receiver')
        return pdu
    else:
        return None
def probeOutgoingCall(klog):
    if klog.uid != 1001 or KlogType[klog.type] != "WRITE":
        return None
    if klog.param.startswith("ATD"):
        return klog.param[3:klog.param_size]
    else:
        return None

def probeIncomingCall(klog, sock):
    if klog.uid != 1001 or KlogType[klog.type] != "WRITE":
        return None
    if klog.param.startswith("AT+CLCC"):
        # Swallow CTL+M
        e = rawStream2Klog(sock.recv(KLOGSIZE))
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # CLCC Response
        while True:
            e = rawStream2Klog(sock.recv(KLOGSIZE)) 
            if not e:
                return None
            printKlogEntry(e)
            print 80*'-'
            if e.uid == 1001 and KlogType[e.type] == 'READ':
                break
        if e.param.startswith("+CLCC"):
            clcc_res = e.param[6:].strip().split(',', 6)
            #print clcc_res
            return clcc_res[5]
        else:
            return None
    else:
        return None
if __name__ == "__main__":
    main()
