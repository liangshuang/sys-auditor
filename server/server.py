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

KlogType = ['WRITE', 'READ', 'OPEN', 'CLOSE']
#******************************** Program Entry ********************************
def main():
    HOST = ''
    #HOST = '129.32.94.230'
    PORT = 8888
    ADDR = (HOST, PORT)

    BUFSIZ = 284
    tcpSerSock = socket(AF_INET, SOCK_STREAM)
    tcpSerSock.bind(ADDR)
    tcpSerSock.listen(5)
    print 'Server started at %s:%d, waiting for connection...' % (HOST, PORT)
    # Wait for new connection, blocking
    tcpCliSock, addr = tcpSerSock.accept()
    print '...connected from ', addr 
    
    klog_fmt = "iiiiiii256s"
    KlogEntry = namedtuple('KlogEntry', 'type hour min sec pid uid param_size param')
    # Print kernel log
    while True:
        log = tcpCliSock.recv(BUFSIZ)
        if not log:
            print 'Sock recv error!'
            continue;
        else:
            if len(log) != BUFSIZ:
                print len(log)
            else:
                e = KlogEntry._make(struct.unpack(klog_fmt, log))
                print 'time: %d:%d:%d' % (e.hour, e.min, e.sec)
                print 'pid: %d, uid: %d' % (e.pid, e.uid)
                print 'type: ', KlogType[e.type]
                print 'param(%d): %s' % (e.param_size, e.param)
                print 80*'-'

    tcpCliSock.close()
        
    tcpSerSock.close()


if __name__ == "__main__":
    main()
