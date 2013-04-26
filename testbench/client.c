/*******************************************************************************
 * client.c
 * Test socket communication between c and python
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "syscall_klog.h"

int main()
{
    int tcpCliSock;
    int ret;
    /* Tcp socket to server */
    tcpCliSock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serAddr;
    //char *serIP = "10.0.2.2";
    char *serIP = "127.0.0.1";
    short serPORT = 8888;
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = inet_addr(serIP);
    serAddr.sin_port = htons(serPORT);

    ret = connect(tcpCliSock, (const struct sockaddr*)&serAddr, sizeof(serAddr)); 
    if(ret == 0) {
        printf("Connected to server %s: %d\n", inet_ntoa(serAddr.sin_addr), serPORT);
    } else {
        printf("Failed to conncet to %s: %d\n", inet_ntoa(serAddr.sin_addr), serPORT);
        exit(-1);
    }

    struct klog_entry e;
    memset(&e, 0, sizeof(struct klog_entry));
    e.ts.hour = 12;
    e.ts.min = 20;
    e.ts.sec = 11;
    e.pid = 888;
    e.type = MYKLOG_WRITE;
    e.uid = 1;
    e.param_size = 16;
    printf("uid: %d\n", e.uid);
    
    // Send to server 
    char *hello = "hello, world!";
    strcpy(e.param, hello);
    //write(tcpCliSock, hello, strlen(hello));
    write(tcpCliSock, &e, sizeof(e));
    close(tcpCliSock);
    return 0;
}

