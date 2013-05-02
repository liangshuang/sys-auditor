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
#include <fcntl.h>
#include "syscall_klog.h"
/****************************** Global Declarations ***************************/
#define MX_UIDS    32
#define MX_KEY_LEN  16
#define KLOG_BUF_SIZE   64

#define FALSE   0
#define TRUE    !FALSE
// Debug
#define DEBUG_KLOG   1

#if DEBUG_KLOG
#define PRINT(fmt, args...)     if(debug_en) printf(fmt, ##args)
#else
#define PRINT(fmt, args...)
#endif
/******************************** Global Variables ****************************/
int UidList[MX_UIDS];
int uid_cnt = 0;

char FilterKey[MX_KEY_LEN];
int key_cnt = 0;

int debug_en = FALSE;
/****************************** Function Protocalls ***************************/
int klog_dump(int in_fd, int out_fd);
//int log_proc(int fd, int size);

/******************************** Program Entry *******************************/
void print_help()
{
    printf("Klog Agent v1.0 for Android Emulator\n");
    printf("Usage:\n");
    printf("\tklogagent <-u uid1> [-u uid2] ... [-u uidn] [-d]\n");
    printf("Options:\n");
    printf("\t-u - specify the app's uids to monitor\n");
    printf("\t-d - enable printing debug messages\n");
    printf("\n");

}
int klogagent_main(int argc, char* argv[])
{
    int tcpCliSock;
    int ret;
    int i;
    /* Initialize */
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
            else if(strcmp(argv[i], "-k") == 0) {
                i++;
                strcpy(FilterKey, argv[i]);
                key_cnt++;
            }

        }
        printf("Klog Agent v1.0 for Android Emulator\n");
        if(uid_cnt > 0) {
            printf("Monitoring Apps: ");
            for(i = 0; i < uid_cnt; i++) {
                printf("%d ", UidList[i]);
            }
            printf("\n");
        }
        if(key_cnt > 0) {
            printf("Filter by key %s\n", FilterKey);
        }
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
    int debugfd = open("/sys/kernel/debug/klogger", O_RDWR);
    printf("%d\n", debugfd);
    //printf("Kernel logs:\n");
    while(!klog_dump(debugfd, tcpCliSock));
    return 0;
}
//------------------------------------------------------------------------------
// Get log info from kernel 
//------------------------------------------------------------------------------
int klog_dump(int in_fd, int out_fd)
{
    int res; 
    struct klog_entry  klog_buf[KLOG_BUF_SIZE];
    //struct klog_entry *pe;
    //struct klog_entry e;
    //char param[PARAM_BUF_SIZE];
    int i = 0;
    //lseek(in_fd, 1, SEEK_SET);    
    res = read(in_fd, klog_buf, KLOG_BUF_SIZE);
    if(res == -1) {
        perror("Read error!!\n");
        return -1;
    }
    if(res == 0) 
        return 0;
    /* Upload logs to server */
    for(i = 0; i < res; i++) {
        if(filter_uid(klog_buf[i].uid) || ( key_cnt > 0 && klog_buf[i].param_size > 0 && filter_key(klog_buf[i].param, FilterKey) ))
            write(out_fd, &klog_buf[i], sizeof(struct klog_entry));
    }
    return 0;
}

//------------------------------------------------------------------------------
// Filter logs by uid
//------------------------------------------------------------------------------
int filter_uid(int uid)
{
    int i;
    if(uid_cnt == 0)
        return TRUE;

    for(i = 0; i < uid_cnt; i++) {
        if(UidList[i] == uid) {
            return TRUE;
        }
    }
    return FALSE;
}

int filter_key(const char *param, const char *key)
{
    if(strstr(param, key))
        return  TRUE;
    else return FALSE;
}

