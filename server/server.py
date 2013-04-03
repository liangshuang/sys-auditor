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
    HOST = 'localhost'
    PORT = 6666
    ADDR = (HOST, PORT)
    print 'Server started, waiting for connection...'
    tcpSerSock = socket(AF_INET, SOCK_STREAM)
    tcpSerSock.bind(ADDR)
    tcpSerSock.listen(5)

    # Wait for new connection, blocking
    tcpCliSock, addr = tcpSerSock.accept()
    print '...connected from ', addr 
    tcpCliSock.close()
    
    tcpSerSock.close()


if __name__ == "__main__":
    main()
