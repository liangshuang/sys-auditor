/*******************************************************************************
 * File: circular_queue.c
 * A circular queue structure for klog logs buffer
 * Author: Lexon <co.liang.ol@gmail.com>
 ******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/stddef.h>
#include <linux/kmod.h>
#include <linux/fs.h>


#include "syscall_klog.h"
#include "klog_queue.h"
#include <asm/uaccess.h>        /* copy_to_user */

/******************************** Definitions *********************************/
#define KLOG_QUEUE_SIZE     (1<<12)

/******************************** Declarations ********************************/
QueueItem KlogQueue[KLOG_QUEUE_SIZE];
int front = 0;
int rear = 0;

/********************************* Prototypes *********************************/

/********************************** Functions *********************************/
int klog_enqueue(QueueItem *item)
{
    if(klog_isfull())
        return 0;
    memcpy(&KlogQueue[rear], item, sizeof(QueueItem));
    rear = (rear + 1)  % KLOG_QUEUE_SIZE;
    printk(KERN_INFO "enqueued, rear: %d\n", rear);
    return 1;
}

int klog_dequeue(QueueItem *buf, int count)
{
    int avail = klog_avail();
    if(count == 0 || avail == 0)
        return 0;
    if(count > avail)
        count = avail;
    /* From front to rear */
    int forward = KLOG_QUEUE_SIZE - front;
    if(count <= forward) {
        memcpy(buf, &KlogQueue[front], sizeof(QueueItem)*count);
    }
    else {
        memcpy(buf, &KlogQueue[front], sizeof(QueueItem)*forward);
        buf += forward;
        memcpy(buf, KlogQueue, sizeof(QueueItem)*(count - forward));
    }
    front = (front + count) % KLOG_QUEUE_SIZE;
    return count;
   
}

int klog_dequeue_to_user(char __user *userbuf, int count) 
{
    int avail = klog_avail();
    int ret;
    /* Check for available elements in the queue  */
    if(count == 0 || avail == 0)
        return 0;
    if(count > avail)
        count = avail;
    /* From front to rear */
    int forward = KLOG_QUEUE_SIZE - front;
    if(count <= forward) {
        ret = copy_to_user(userbuf, &KlogQueue[front], sizeof(QueueItem)*count);
        count -= ret;
    }
    /* Read wrap back */
    else {
        ret = copy_to_user(userbuf, &KlogQueue[front], sizeof(QueueItem)*forward);
        ret += copy_to_user(userbuf+forward, KlogQueue, sizeof(QueueItem)*(count - forward)); 
        count -= ret;
    }
    front = (front + count) % KLOG_QUEUE_SIZE;
    printk("dequeued, front: %d\n", front);
    return count;
}

int klog_isempty()
{
    return (front == rear);
}

int klog_isfull()
{
    return (((rear+1) % KLOG_QUEUE_SIZE) == front);
}

int klog_avail()
{
    return rear >= front ? (rear - front) : \
                                    (KLOG_QUEUE_SIZE - front + rear);
}

