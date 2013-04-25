#ifndef KLOG_QUEUE_H
#define KLOG_QUEUE_H

/******************************** Exported Definitions ************************/
typedef struct klog_entry   QueueItem;

/********************************* Exported Functions *************************/
int klog_enqueue(QueueItem *item);
int klog_dequeue(QueueItem *buf, int count);
int klog_isempty(void);
int klog_isfull(void);
int klog_avail(void);
int klog_dequeue_to_user(char __user *userbuf, int count);
#endif // KLOG_QUEUE_H

