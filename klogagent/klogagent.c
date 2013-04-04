/*
 * File: klogtrans.c 
 * Filter and transfer klog to auditing server
 * Author: Shuang Liang <shuang.liang2012@temple.edu>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/klog.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


/*********************** Global Variables and Declarations ********************/
#define LOGBUFFER_SIZE 1024
char log_buffer[LOGBUFFER_SIZE];

/************************** Function Protocalls *******************************/
int klog_dump(int fd);
int log_proc(int fd, int size);

/******************************** Program Entry *******************************/
int klogagent_main(int argc, char* argv[])
{
    int tcpCliSock;
    int ret;
    tcpCliSock = socket(AF_INET, SOCK_STREAM, 0);
    memset(log_buffer, 0, sizeof(log_buffer));
    /* Connect to the server using TCP socket */
    struct sockaddr_in serAddr;
    //char *serIP = "10.0.2.2";
    //char *serIP = "24.238.102.111";
    char *serIP = "129.32.94.230";
    short serPORT = 8888;
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = inet_addr(serIP);
    serAddr.sin_port = htons(serPORT);
    ret = connect(tcpCliSock, (const struct sockaddr*)&serAddr, sizeof(serAddr)); 
    if(ret == 0) {
        printf("Connected to server %s:%d\n", inet_ntoa(serAddr.sin_addr.s_addr), serPORT);
    } else {
        printf("Failed to conncet to %s:%d\n", inet_ntoa(serAddr.sin_addr.s_addr), serPORT);
        exit(-1);
    }

    /* Transer logs to server */
    //int fd = STDOUT_FILENO;
    printf("Kernel logs:\n");
    while(!klog_dump(tcpCliSock));
    return 0;
}
//------------------------------------------------------------------------------
// Get log info from kernel 
//------------------------------------------------------------------------------
int klog_dump(int fd)
{
/*
 * Arguments to klogctl:
 *
 *  0 -- Close the log.  Currently a NOP.
 *  1 -- Open the log. Currently a NOP.
 *  2 -- Read from the log.
 *  3 -- Read all messages remaining in the ring buffer.
 *  4 -- Read and clear all messages remaining in the ring buffer
 *  5 -- Clear ring buffer.
 *  6 -- Disable printk to console
 *  7 -- Enable printk to console
 *  8 -- Set level of messages printed to console
 *  9 -- Return number of unread characters in the log buffer
 *  10 -- Return size of the log buffer
*/

    int logcnt = 0;
    logcnt = klogctl(2, log_buffer, sizeof(log_buffer)); 
    if(logcnt < 0) {
        if(errno == EINTR) {
            return 0;
        } else {
            //perror()
            printf(stderr, "ERROR klogctl\n");
            close(fd);
            return -1;
        }
    } else return log_proc(fd, logcnt);

}
//------------------------------------------------------------------------------
// Preprocess logs before send out
//------------------------------------------------------------------------------
int log_proc(int fd, int size)
{
    int len = size;
    int rc = 0;
    char* pbuf = log_buffer;
    while(len) {
        rc = write(fd, pbuf, len);
        if(rc == -1) {
            if(errno == EINTR) continue;
            else {
                close(fd);
                return -1;
            }
        }
        len -= rc;
        pbuf += rc;
        fsync(fd);
    }
    return 0;
}

