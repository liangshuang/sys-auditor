/*
 * File: syscall_logger.c
 * Log system calls invoked by processes of malware
 * Author: Shuang Liang <co.liang.ol@gmail.com>
 * Android Linux Kernel Version: 2.6.29
 */

#include <asm/unistd.h>
#include <linux/autoconf.h>
#include <linux/in.h>
#include <linux/init_task.h>
#include <linux/ip.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/version.h>
#include <linux/workqueue.h>

#include <linux/time.h>

asmlinkage ssize_t (*orig_read)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_write)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_open)(const char *pathname, int flags);
asmlinkage ssize_t (*orig_close)(int fd);

struct time_m{
    int hour;
    int min;
    int sec;
};

struct time_m get_time()
{
    unsigned long get_time;
    int sec, hr, min, tmp1,tmp2;
    struct timeval tv;
    struct time_m mytime;

    do_gettimeofday(&tv);
    get_time = tv.tv_sec;
    sec = get_time % 60;
    tmp1 = get_time / 60;
    min = tmp1 % 60;
    tmp2 = tmp1 / 60;
    hr = tmp2 % 24;
    mytime.hour = hr;
    mytime.min = min;
    mytime.sec = sec;
    return mytime;
}
//------------------------------------------------------------------------------
// Hooked write for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t 
logger_write(int fd, char *buf, size_t count)
{
    struct time_m mytime = get_time();
    printk(KERN_INFO "%d:%d:%d\tlogger_write: %s\n", mytime.hour, \
            mytime.min, mytime.sec, buf);
    return orig_write(fd, buf, count);
}

//------------------------------------------------------------------------------
// Hooked read for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_read(int fd, char *buf, size_t count)
{
    printk(KERN_INFO "Logger_read: %s\n", buf);
    return orig_read(fd, buf, count);
}

//------------------------------------------------------------------------------
// hooked open for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_open(const char *pathname, int flags)
{
    printk(KERN_INFO "Logger_open: %s\n", pathname);
    return orig_open(pathname, flags);
}

//------------------------------------------------------------------------------
// hooked close for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_close(int fd)
{
    printk(KERN_INFO "Logger_close: %s\n", current->comm);
    return orig_close(fd);
}

//------------------------------------------------------------------------------
// Initialize and start system call hooker
//------------------------------------------------------------------------------
static int __init
logger_start(void)
{
    unsigned long *sys_call_table = (unsigned long*)0xc0027f84;
    orig_read = sys_call_table[__NR_read];
    sys_call_table[__NR_read] = logger_read;
    orig_write = sys_call_table[__NR_write];
    sys_call_table[__NR_write] = logger_write;
    orig_open = sys_call_table[__NR_open];
    sys_call_table[__NR_open] = logger_open;
    orig_close = sys_call_table[__NR_close];
    sys_call_table[__NR_close] = logger_close;
    return 0;
}

//------------------------------------------------------------------------------
// Stop logger and restore sys_call_table
//------------------------------------------------------------------------------
static void __exit
logger_stop(void)
{
    unsigned long *sys_call_table = (unsigned long*)0xc0027f84;
    sys_call_table[__NR_read] = &orig_read;
    sys_call_table[__NR_write] = &orig_write;
    sys_call_table[__NR_open] = &orig_open;
    sys_call_table[__NR_close] = &orig_close;

}

module_init(logger_start);
module_exit(logger_stop);

