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

#include <string.h>
/*********************** Global Variables and Declarations ********************/
#define LOGBUFFER_SIZE 4096

char log_buffer[LOGBUFFER_SIZE];

/************************** Function Protocalls *******************************/
int klog_dump(int fd);
int log_proc(int fd, int size);

/******************************** Program Entry *******************************/
int klogagent_main(int argc, char* argv[])
{
    int tcpCliSock;
    int ret;
    memset(log_buffer, 0, sizeof(log_buffer));
    /* Connect to the server using TCP socket */
    /*
    tcpCliSock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serAddr;
    char *serIP = "10.0.2.2";
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
    */
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
    int loglen = size;
    int rc = 0;
    char* pbuf = log_buffer;
    char filter_buffer[LOGBUFFER_SIZE];
    char *entry, *entry_end;
    /* Filter based on UID */
    int log_level;
    char ktag[16], *ptag;
    char *local_time;
    int pid, uid;
    char *syscall;
    char temp[32], *ptemp;
    int len;

    entry_end = pbuf;
    printf("Read kbuffer %d\n", loglen);
    while(loglen>0) {
        while(*entry_end++ != '<') loglen--;
        entry = entry_end-1;
        // log-level
        log_level = *entry_end++ - '0';
        printf("log-level=%d\n", log_level);

        while(*entry_end++ != '>') loglen--;
        // timestamp
        while(*entry_end++ != '[') loglen--;
        while(*entry_end++ != ']') loglen--;

        entry_end++; loglen--;   // blank
        // KLOG_TAG
        if(*entry_end++ != '[') {
            loglen--; continue;
        }
        ptag = entry_end;
        while(*entry_end++ != ']') loglen--;
        len = entry_end-1-ptag;
        memcpy(ktag, ptag, len);
        *(ktag+len) = '\0';
        printf("TAG=%s\n", ktag);
        if(strcmp(ktag, "KLOG") != 0)  continue;
        // local-time
        while(*entry_end++ != '[') {
            loglen--;
            if(!loglen) goto end;
        }
        while(*entry_end++ != ']') {
            loglen--;
            if(!loglen) goto end;
        }
        // PID
        while(*entry_end++ != '[') {
            loglen--;
            if(!loglen) goto end;
        }
        ptemp = entry_end;
        while(*entry_end++ != ']') {
            loglen--;
            if(!loglen) goto end;
        }
        len = entry_end - ptemp -1; 
        memcpy(temp, ptemp, len);
        *(ptemp+len) = '\0';
        printf("PID=%d\n", atoi(ptemp));
        // UID
        while(*entry_end++ != '[') {
            loglen--;
            if(!loglen) goto end;
        }
        ptemp = entry_end;
        while(*entry_end++ != ']') {
            loglen--;
            if(!loglen) goto end;
        }
        len = entry_end - ptemp -1; 
        memcpy(temp, ptemp, len);
        *(ptemp+len) = '\0';
        printf("UID=%d\n", atoi(ptemp));
        // system-call
        while(*entry_end++ != '[') {
            loglen--;
            if(!loglen) goto end;
        }
        ptemp = entry_end;
        while(*entry_end++ != ']') {
            loglen--;
            if(!loglen) goto end;
        }
        len = entry_end - ptemp -1;
        memcpy(temp, ptemp, len);
        *(ptemp+len) = '\0';
        printf("SYSCALL=%s\n", ptemp);
        // parameters 
        while(*entry_end++ != '[') {
            loglen--;
            if(!loglen) goto end;
        }
        while(*entry_end++ != ']') {
            loglen--;
            if(!loglen) goto end;
        }
end:
    printf("Not completed\n");
    }
    return 0;
    while(loglen) {
        /* Add filter */
         
        /* Send log to server */
        rc = write(fd, pbuf, loglen);
        if(rc == -1) {
            if(errno == EINTR) continue;
            else {
                close(fd);
                return -1;
            }
        }
        loglen -= rc;
        pbuf += rc;
        fsync(fd);
    }
    return 0;
}

