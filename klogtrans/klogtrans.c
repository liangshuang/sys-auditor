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
/*********************** Global Variables and Declarations ********************/
#define LOGBUFFER_SIZE 1024
char log_buffer[LOGBUFFER_SIZE];

/************************** Function Protocalls *******************************/
int klog_copy(void);
int log_proc(int size);

/******************************** Program Entry *******************************/
int klogtrans_main(int argc, char* argv[])
{
    memset(log_buffer, 0, sizeof(log_buffer));
    printf("Kernel logs:\n");
    while(!klog_copy());
    return 0;
}
//------------------------------------------------------------------------------
// Get log info from kernel 
//------------------------------------------------------------------------------
int klog_copy()
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
        }
        //perror()
        printf(stderr, "ERROR klogctl\n");
        return -1;
    } else return log_proc(logcnt);

}
//------------------------------------------------------------------------------
// Preprocess logs before send out
//------------------------------------------------------------------------------
int log_proc(int size)
{
    int len = size;
    int rc = 0;
    char* bp = log_buffer;
    printf("Display buffer content %d: \n", len);
    while(len) {
        //rc = write(STDOUT_FILENO, bp, len);
        if(rc == -1) {
            if(errno == EINTR) continue;
            else return -1;
        }
        len -= rc;
        bp += rc;
        fsync(STDOUT_FILENO);
    }
    return 0;
}
