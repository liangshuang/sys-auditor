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
/****************************** Global Declarations ***************************/
#define MX_UIDS    32
#define LOGBUFFER_SIZE 4096

#define FALSE   0
#define TRUE    !FALSE
// Debug
#define DEBUG_KLOG   1
#if DEBUG_KLOG
#define PRINT(enable, fmt, args...)     if(enable) printf(fmt, ##args)
#else
#define PRINT(enable, fmt, args...)
#endif
/******************************** Global Variables ****************************/
char log_buffer[LOGBUFFER_SIZE+1];
int UidList[MX_UIDS];
int uid_cnt = 0;

int debug_en = FALSE;
/************************** Function Protocalls *******************************/
int klog_dump(int fd);
int log_proc(int fd, int size);

/******************************** Program Entry *******************************/
void print_help()
{
    printf("Klog Agent v1.0 for Android Emulator\n");
    printf("Usage:\n");
    printf("klogagent <uid 1> [uid 2] ... [uid n]\n");
    printf("\n");

}
int klogagent_main(int argc, char* argv[])
//int main(int argc, char* argv[])
{
    int tcpCliSock;
    int ret;
    int i;
    /* Initialize */
    memset(log_buffer, 0, sizeof(log_buffer));
    memset(UidList, 0, sizeof(UidList));
    /* Parse command parameters */
    switch(argc){
    case 1:
        print_help();
        return 0;
    default:
        for(i = 1; i < argc; i++) {
            if(strcmp("-u", argv[i]) == 0) {
                i++;
                UidList[uid_cnt++] = atoi(argv[i]);
            }
            else if(strcmp(argv[i], "-d") == 0) {
                debug_en = TRUE;
            }
        }
        printf("Klog Agent v1.0 for Android Emulator\n");
        printf("Monitoring Apps: ");
        for(i = 0; i < uid_cnt; i++) {
            printf("%d ", UidList[i]);
        }
        printf("\n");
        if(debug_en) printf("Debug enabled.\n");
        printf("\n");
    }
    /* Connect to the server using TCP socket */
    //*
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
    //*/
    /* Transer logs to server */
    //int fd = STDOUT_FILENO;
    //printf("Kernel logs:\n");
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
    /* Read from kernel buffer */
    logcnt = klogctl(2, log_buffer, sizeof(log_buffer)); 
    /* Read from internal buffer */

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

int filter_uid(int uid)
{
    int i;
    for(i = 0; i < uid_cnt; i++) {
        if(UidList[i] == uid) {
            return TRUE;
        }
    }
    return FALSE;
}

int log_proc(int fd, int size)
{
    int loglen = size;
    int rc = 0;
    char* pbuf = log_buffer;

    char filter_buffer[LOGBUFFER_SIZE];
    char *pfbuf = filter_buffer;
    char *entry, *entry_end;
    /* Filter based on UID */
    int log_level;
    char ktag[16], *ptag;
    char *local_time;
    int pid, uid;
    char *syscall;
#define MAX_FIELD_LEN   32
    char temp[MAX_FIELD_LEN+1], *ptemp;
    int len;
   
    entry_end = pbuf;
    //printf("******************** Read kbuffer %d***************************\n", loglen);
    while(loglen>0) {
        while(loglen && *entry_end != '<') {
            entry_end++; loglen--;
        };
        if(!loglen) break;
        entry = entry_end;  // Start of next entry
        entry_end++; loglen--;

        // log-level
        log_level = *entry_end - '0'; 
        entry_end++; loglen--;
        
        PRINT(debug_en, "log-level=%d\n", log_level);

        while(loglen && *entry_end != '>') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;

        // timestamp
        while(loglen && *entry_end != '[') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;

        while(loglen && *entry_end != ']') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        // KLOG_TAG
        while(loglen && *entry_end != '[') {
            entry_end++; loglen--;;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        ptag = entry_end;

        while(loglen && *entry_end != ']') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;

        len = entry_end-1-ptag;
        if(len > MAX_FIELD_LEN) {
            PRINT(debug_en, "TAG entry exceeds buffer size\n");
            continue;
        }

        memcpy(ktag, ptag, len);
        *(ktag+len) = '\0';
        PRINT(debug_en, "TAG=%s\n", ktag);
        if(strcmp(ktag, "KLOG") != 0)  {
            PRINT(debug_en, "Not KLOG but %s\n", ktag);
            continue;
        }

        // local-time
        while(loglen && *entry_end != '[') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        ptemp = entry_end;
        
        while(loglen && *entry_end != ']') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        len = entry_end - ptemp - 1;
        if(len > MAX_FIELD_LEN) {
            PRINT(debug_en, "Localtime entry exceeds buffer size\n");
            continue;
        }
        memcpy(temp, ptemp, len);
        ptemp = temp;
        *(ptemp+len) = '\0';
        PRINT(debug_en, "LocalTime=%s\n", ptemp);
        // PID
        while(loglen && *entry_end != '[') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        entry_end+=5; loglen-=5;  // Skip PID:
        ptemp = entry_end;

        while(loglen && *entry_end != ']')  {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        len = entry_end - ptemp -1; 
        if(len > MAX_FIELD_LEN) {
            PRINT(debug_en, "PID entry exceeds buffer size\n");
            continue;
        }
        memcpy(temp, ptemp, len);
        ptemp = temp;
        *(ptemp+len) = '\0';
        pid = atoi(ptemp);
        PRINT(debug_en, "PID=%d\n", pid);
        // UID
        while(loglen && *entry_end != '[') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        entry_end+=5; loglen-=5;  // Skip UID: 
        ptemp = entry_end;
        while(loglen && *entry_end != ']') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        len = entry_end - ptemp -1; 
        if(len > MAX_FIELD_LEN) {
            PRINT(debug_en, "UID entry exceeds buffer size\n");
            continue;
        }

        memcpy(temp, ptemp, len);
        ptemp = temp;
        *(ptemp+len) = '\0';
        uid = atoi(ptemp);
        PRINT(debug_en, "UID=%d\n", uid);
        // system-call
        while(loglen && *entry_end != '[') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        ptemp = entry_end;
        while(loglen && *entry_end != ']')  {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        len = entry_end - ptemp -1;
        if(len > MAX_FIELD_LEN) {
            PRINT(debug_en, "SYSCALL entry exceeds buffer size\n");
            continue;
        }

        memcpy(temp, ptemp, len);
        ptemp = temp;
        *(ptemp+len) = '\0';
        PRINT(debug_en, "SYSCALL=%s\n", ptemp);
        // parameters 
        char param_buf[512];
        while(loglen && *entry_end != '[') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;

        ptemp = entry_end;
        while(loglen && *entry_end != ']') {
            entry_end++; loglen--;
        }
        if(!loglen) break;
        entry_end++; loglen--;
        len = entry_end - ptemp -1;

        if(len > 512) {
            PRINT(debug_en, "SYSCALL entry exceeds buffer size\n");
            continue;
        }
        memcpy(param_buf, ptemp, len);
        ptemp = param_buf;
        *(ptemp+len) = '\0';
        /* Filter by uid */
        if(filter_uid(uid)) {
            len = entry_end - entry + 1;
            memcpy(pfbuf, entry, len);
            pfbuf += len;
        }
        /* Filter by paramater */
        if(strstr(param_buf, "AT")) {
            len = entry_end - entry + 1;
            memcpy(pfbuf, entry, len);
            pfbuf += len;
        }

    }
    /* Transfer filterred logs to server */
    loglen = pfbuf-filter_buffer;
    pfbuf = filter_buffer;
    while(loglen) {
        /* Add filter */
         
        /* Send log to server */
        rc = write(fd, pfbuf, loglen);
        if(rc == -1) {
            if(errno == EINTR) continue;
            else {
                close(fd);
                return -1;
            }
        }
        loglen -= rc;
        pfbuf += rc;
        fsync(fd);
    }
    return 0;
}

