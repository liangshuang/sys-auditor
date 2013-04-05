#!/usr/bin/env python

# 
# File: server.py
# Get kernel logs from clients, and apply analysis methods
# Author: Shuang Liang <shuang.liang2012@temple.edu>
#

#******************************** Modules **************************************
from socket import *


#******************************** Program Entry ********************************
def main():
    HOST = ''
    #HOST = '129.32.94.230'
    PORT = 8888
    ADDR = (HOST, PORT)

    BUFSIZ = 4096
    tcpSerSock = socket(AF_INET, SOCK_STREAM)
    tcpSerSock.bind(ADDR)
    tcpSerSock.listen(5)
    print 'Server started at %s:%d, waiting for connection...' % (HOST, PORT)
    # Wait for new connection, blocking
    tcpCliSock, addr = tcpSerSock.accept()
    print '...connected from ', addr 

    # Print kernel log
    while True:
        log = tcpCliSock.recv(BUFSIZ)
        if not log:
            continue;
        else:
            print log

    tcpCliSock.close()
        
    tcpSerSock.close()


if __name__ == "__main__":
    main()
