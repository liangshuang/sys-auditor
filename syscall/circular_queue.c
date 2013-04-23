/*******************************************************************************
 * File: circular_queue.c
 * A circular queue structure for klog logs buffer
 * Author: Lexon <co.liang.ol@gmail.com>
 ******************************************************************************/
#include "syscall_klog.h"
/******************************** Definitions *********************************/
#define MAX_KLOG_QUEUE_SIZE     (1<<10)

typedef struct klog_entry   QueueItem;
/******************************** Declarations ********************************/
QueueItem KlogQueue[MAX_KLOG_QUEUE_SIZE];
int head = 0;
int tail = 0;

/************************************ Functions *******************************/

