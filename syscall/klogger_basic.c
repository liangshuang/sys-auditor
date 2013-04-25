/*******************************************************************************
 * File: syscall_logger.c
 * Log system calls invoked by processes of malware
 * Author: Shuang Liang <co.liang.ol@gmail.com>
 * Android Linux Kernel Version: 2.6.29
 ******************************************************************************/

#include <linux/in.h>
#include <linux/init_task.h>
#include <linux/ip.h>
#include <linux/kmod.h>
#include <linux/mm.h>
#include <linux/skbuff.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/kfifo.h>
#include <linux/fs.h>

#include "klogger.h"
#include "klog_queue.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lexon");
/********************************* Definitions ********************************/

/********************************** Declarations  *****************************/

#define KLOG_QUEUE_SIZE     ((1<<20))   /* 512KB */
char Queue_buf[KLOG_QUEUE_SIZE] = {0};

struct kfifo klog_fifo;
struct dentry* dir, *debugfile;

/********************************** Protocalls ********************************/

/******************************** Program Entry  ******************************/
static ssize_t logger_read(struct file *file, char __user *userbuf,
                                size_t count, loff_t *ppos)
{
    loff_t pos = *ppos;
    size_t ret;
    //return simple_read_from_buffer(userbuf, count, ppos, mybuf, 200);
    static char tmp[KLOG_QUEUE_SIZE] = {0};
    //printk(KERN_WARNING "klogger: read by debugfs...\n");
    size_t available = kfifo_len(&klog_fifo);

    if(pos < 0)   
        return -EINVAL;
    if(pos >= available || !count) 
        return 0;
    /* Read from queue start */ 
    if(pos == 0) {
        if(count > available - pos) 
            count = available - pos;
        kfifo_out(&klog_fifo, tmp, count);
    }
    /* Write to user buffer */
    ret = copy_to_user(userbuf, tmp+pos, count);
    if(ret == count) 
        return -EFAULT;
    count -= ret;
    *ppos = pos + count;
    //printk(KERN_WARNING "klogger: read end\n");
    return count;
} 

//-----------------------------------------------------------------------------
// Modified read for klog queue
//-----------------------------------------------------------------------------
static ssize_t logger_block_read(struct file *file, char __user *userbuf, 
                                    size_t count, loff_t *ppos)
{
    int avail = klog_avail();
    if(count == 0 || avail == 0)
        return 0;
    if(count > avail) 
        count = avail;

    printk("Read klogs: %d\n", count);
    count = klog_dequeue_to_user(userbuf, count);
    return count;
}

static ssize_t logger_write(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
    static char tmp[KLOG_QUEUE_SIZE] = {0};
    loff_t pos = *ppos;
    size_t available = kfifo_avail(&klog_fifo);
    size_t ret;
    /*
    if(count > 200)
        return -EINVAL;
    copy_from_user(mybuf, buf, count);
    return count;
    */
    if(pos < 0)
        return -EINVAL;
    if(pos >= available || !count) {
        kfifo_reset(&klog_fifo);
        return 0;
    }

    if(pos == 0) {
        if(count > available - pos)
            count = available - pos;
        ret = copy_from_user(tmp+pos, buf, count);
        if(ret == count)
            return -EFAULT;
        count -= ret;
        //*ppos = pos + count;
    }
    ret = kfifo_in(&klog_fifo, tmp+pos, count);
    *ppos = pos + ret;
    count = ret;
    return count;

}

ssize_t logfifo_write(const char *addr, size_t count)
{
    size_t available = kfifo_avail(&klog_fifo);
    //*
    if(count > available || !count) {
        kfifo_reset(&klog_fifo);
        return 0;
    }
    //*/
    count = kfifo_in(&klog_fifo, addr, count);
    return count;
}

static const struct file_operations logger_fops = {
    //.read = logger_read,
    .read = logger_block_read,
    .write = logger_write
};

//------------------------------------------------------------------------------
// Module I/F
//------------------------------------------------------------------------------
static int __init klogger_init(void)
{
    /* Initialize kfifo for logs */
    kfifo_init(&klog_fifo, Queue_buf, KLOG_QUEUE_SIZE);
    /* Debugfs */ 
    debugfile = debugfs_create_file("klogger", 0644, NULL, NULL, &logger_fops);
    /* System call hooks */
    hook_start();

    printk(KERN_INFO "Create klogger\b");
    return 0;
}

static void __exit klogger_exit(void)
{
    hook_stop();
    debugfs_remove(debugfile);

    printk(KERN_INFO "Remove klogger\n");
}

module_init(klogger_init);
module_exit(klogger_exit);

