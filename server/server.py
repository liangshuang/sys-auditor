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
KlogType = ['PRINT', 'WRITE', 'READ', 'OPEN', 'CLOSE', 'SOCKETCALL',
            'SOCKET', 'BIND', 'CONNECT', 'LISTEN', 'ACCEPT', 'SEND', 'SENDTO',
            'RECV', 'RECVFROM']
PremiumPrefix = ('900', '1900')
KLOGSIZE = 284        # Size of C struct klog_entry
LOG2FILE = True


WARN_SMS = 1
WARN_CALL = 2
WARN_PREMIUM_SMS = 3
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

    # Wait for Klog agent connect, blocking
    tcpKlogAgentSock, addr = tcpSerSock.accept()
    print 'Connected from Klog agent ', addr

    # Wait for App agent connect, blocking
    tcpAppAgentSock, addr = tcpSerSock.accept()
    print 'Connected from App agent ', addr 
    # Start klog agent
    tcpKlogAgentSock.send("start")
    # Start recording and analyzing
    klogRecFile = open("klogs.txt", "w") 
    # Create socket to communicate with App Agent
    while True:
        logbuf = tcpKlogAgentSock.recv(KLOGSIZE)
        e = rawStream2Klog(logbuf, tcpKlogAgentSock)
        if not e:
            print 'Sock recv error!'
            # send back nack
            continue;
        else:
            checkRes = None
            writeKlogToFile(klogRecFile, e)
            if e.param and e.param.startswith("AT"):
                checkRes = checkTelephony(e, tcpKlogAgentSock)
            if checkRes:
                # Get current active UID
                #code = 0
                rc = tcpAppAgentSock.send(struct.pack('!i', 0))
                uidbuf = tcpAppAgentSock.recv(4)
                uid = struct.unpack('!i', uidbuf)
                print 'Active App: ', uid
                dest_num = checkRes[1]
                if checkRes[0] == AT_SMS_SUBMIT: #or checkRes == AT_SMS_DELIVER:
                    if uid[0] != 10030:
                        print 'Send SMS [%s] in background to %s' % (checkRes[2], checkRes[1])
                        code = WARN_SMS
                        if isPremium(dest_num):         
                            code = WARN_PREMIUM_SMS
                        tcpAppAgentSock.send(struct.pack('!i', code))
                        tcpAppAgentSock.send(checkRes[1])
                        #tcpAppAgentSock.send(checkRes[2])
                elif checkRes[0] == AT_OUTCALL:
                    if uid[0] != 1001 and uid[0] != 10002:
                        print 'Make phone call in background to ', checkRes[1]
                        tcpAppAgentSock.send(struct.pack('!i', WARN_CALL))
                        tcpAppAgentSock.send(checkRes[1])

                # 
                #print 'Send alert to App Agent %d' % (rc)

    klogRecFile.close()
    tcpKlogAgentSock.close()
    tcpSerSock.close()
def isPremium(num):
    #for p in PremiumPrefix:
    if num.startswith(PremiumPrefix):
        return True
    return False
#-------------------------------------------------------------------------------
# Check the signatures of sms and call related signatures
#-------------------------------------------------------------------------------
AT_SMS_SUBMIT = 1
AT_SMS_DELIVER = 2
AT_OUTCALL = 3
AT_INCALL = 4
def checkTelephony(e, tcpCliSock):
    # Ignore AT+CSQ
    if e.param.startswith("AT+CSQ"):
        return None
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
        #klogRecFile.write('Send SMS [%s] to %s\n' % (pdu.user_data, pdu.tp_address))
        #klogRecFile.write(80*'='+'\n')
        return (AT_SMS_SUBMIT, pdu.tp_address, pdu.user_data)
    # Probe SMS-DELIVER
    pdu = probeSmsReceive(e, tcpCliSock)
    if pdu:
        print 'Receive SMS [%s] from %s' % (pdu.user_data, pdu.tp_address)
        #klogRecFile.write('Receive SMS [%s] from %s\n' % (pdu.user_data, pdu.tp_address))
        #klogRecFile.write(80*'='+'\n')
        return (AT_SMS_DELIVER, pdu.tp_address, pdu.user_data)
    # Probe outgoing call by 'ATD'
    dst_num = probeOutgoingCall(e)
    if dst_num:
        print 'Calling %s' % (dst_num)
        #klogRecFile.write('Calling %s\n' % (dst_num))
        #klogRecFile.write(80*'='+'\n')
        return (AT_OUTCALL, dst_num)
    # Probe incoming call
    clcc_response = probeIncomingCall(e, tcpCliSock)
    if clcc_response:
        if clcc_response[0] == 0:   #MO
            print 'Outgoing call %s' % (clcc_response[1])
            #klogRecFile.write('Outgoing call %s\n' % (clcc_response[1]))
            #klogRecFile.write(80*'='+'\n')
            return (AT_OUTCALL, clcc_response[1])
        else:
            print 'Incoming call %s' % (clcc_response[1])
            #klogRecFile.write('Incoming call %s\n' % (clcc_response[1]))
            #klogRecFile.write(80*'='+'\n')
            return (AT_INCALL, clcc_response[1])
    return None 

def rawStream2Klog(logbuf, sock):
    klog_fmt = "iiiiiii256s"
    KlogEntry = namedtuple('KlogEntry', 'type hour min sec pid uid param_size param')

    if not logbuf or len(logbuf) != KLOGSIZE:
        sock.send('n')
        return None
    e = KlogEntry._make(struct.unpack(klog_fmt, logbuf))

    if not  getKlogType(e):
        sock.send('n')
        return None

    sock.send('a')
    return e

def getKlogType(klog):
    if klog.type > len(KlogType)-1 or klog.type < 0:
        print 'type error!'
        return None
    else:
        return KlogType[klog.type]
    
def printKlogEntry(klog):
    print 'time: %d:%d:%d' % (klog.hour, klog.min, klog.sec)
    print 'pid: %d, uid: %d' % (klog.pid, klog.uid)
    print 'type: ', KlogType[klog.type]
    print 'param(%d): %s' % (klog.param_size, klog.param)

def writeKlogToFile(fp, klog):
    if LOG2FILE:
        fp.write('time: %d:%d:%d\n' % (klog.hour, klog.min, klog.sec)) 
        fp.write('pid: %d, uid: %d\n' % (klog.pid, klog.uid))
        fp.write('type: %s\n' % KlogType[klog.type])
        fp.write('param(%d): %s\n' % (klog.param_size, klog.param[:klog.param_size]))
        fp.write(80*'-'+'\n')

def probeSmsSubmit(klog, sock):

#    if klog.uid != 1001 or KlogType[klog.type] != "WRITE":
#        return None
    if klog.param.startswith("AT+CMGS="):
        # Swallow <CR>
        e = rawStream2Klog(sock.recv(KLOGSIZE), sock)
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # SMSC.addr + TPDU
        while True:
            e = rawStream2Klog(sock.recv(KLOGSIZE), sock) 
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

#-------------------------------------------------------------------------------
# 
#-------------------------------------------------------------------------------
def probeSmsReceive(klog, sock):
#    if klog.uid != 1001 or not klog.param:
#        return None
    if klog.param.startswith("AT+CNMA=1"):
        # Swallow CTL+M
        e = rawStream2Klog(sock.recv(KLOGSIZE), sock)
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # Polling for SMS-DELIVER PDU
        count = 5
        while count:
            e = rawStream2Klog(sock.recv(KLOGSIZE), sock)
            if not e:
                return None
            printKlogEntry(e)
            print 80*'-'
            if e.uid == 1001 and KlogType[e.type] == 'READ':
                break
            count = count - 1
        if count == 0:
            return None
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
        e = rawStream2Klog(sock.recv(KLOGSIZE), sock)
        if not e:
            return None
        printKlogEntry(e)
        print 80*'-'
        # CLCC Response
        count = 5
        while count:
            e = rawStream2Klog(sock.recv(KLOGSIZE), sock) 
            if not e:
                return None
            printKlogEntry(e)
            print 80*'-'
            if e.uid == 1001 and KlogType[e.type] == 'READ':
                break
            count = count - 1
        if not count:
            return None
        if e.param.startswith("+CLCC"):
            clcc_res = e.param[6:].strip().split(',', 6)
            #print clcc_res
            direction = int(clcc_res[1])
            
            return (direction, clcc_res[5])
        else:
            return None
    else:
        return None

if __name__ == "__main__":
    main()
