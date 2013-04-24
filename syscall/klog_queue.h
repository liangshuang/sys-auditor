#ifndef KLOG_QUEUE_H
#define KLOG_QUEUE_H

/******************************** Exported Definitions ************************/
typedef struct klog_entry   QueueItem;

/********************************* Exported Functions *************************/
int klog_enqueue(QueueItem *item);
int klog_dequeue(QueueItem *buf, int count);
int klog_isempty();
int klog_isfull();
int klog_avail();

#endif // KLOG_QUEUE_H

