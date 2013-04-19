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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lexon");
/********************************* Definitions ********************************/

/********************************** Declarations  *****************************/

#define KLOG_QUEUE_SIZE     512 //((2<<10)*512)   /* 512KB */
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
    char tmp[KLOG_QUEUE_SIZE] = {0};
    size_t available = kfifo_len(&klog_fifo);

    if(pos < 0)   
        return -EINVAL;
    if(pos >= available || !count) 
        return 0;
    
    if(pos == 0) {
        if(count > available - pos) 
            count = available - pos;
        kfifo_out(&klog_fifo, tmp, count);
    }
    ret = copy_to_user(userbuf, tmp+pos, count);
    if(ret == count) 
        return -EFAULT;
    count -= ret;
    *ppos = pos + count;
    return count;
} 

static ssize_t logger_write(struct file *file, const char __user *buf,
                                size_t count, loff_t *ppos)
{
    /*
    if(count > 200)
        return -EINVAL;
    copy_from_user(mybuf, buf, count);
    return count;
    */
}

static const struct file_operations logger_fops = {
        .read = logger_read,
        .write = logger_write,
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

    printk(KERN_INFO "Create klogger\b");
    return 0;
}

static void __exit klogger_exit(void)
{
    debugfs_remove(debugfile);
    printk(KERN_INFO "Remove klogger\n");
}

module_init(klogger_init);
module_exit(klogger_exit);

