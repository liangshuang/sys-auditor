/*
 * File: syscall_logger.c
 * Log system calls invoked by processes of malware
 * Author: Shuang Liang <co.liang.ol@gmail.com>
 * Android Linux Kernel Version: 2.6.29
 */

#include <asm/unistd.h>
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
#include <linux/version.h>
#include <linux/workqueue.h>

#include <linux/time.h>

MODULE_LICENSE("GPL");

asmlinkage ssize_t (*orig_read)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_write)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_open)(const char *pathname, int flags);
asmlinkage ssize_t (*orig_close)(int fd);

asmlinkage int (*orig_socketcall)(int call, unsigned long *args);
asmlinkage int (*orig_socket)(int family, int type, int protocol);

struct time_m{
    int hour;
    int min;
    int sec;
};

struct time_m get_time(void)
{
    unsigned long time_secs;
    int sec, hr, min, tmp1,tmp2;
    struct timeval tv;
    struct time_m mytime;

    do_gettimeofday(&tv);
    //*
    time_secs = tv.tv_sec;
    sec = time_secs % 60;
    tmp1 = time_secs / 60;
    min = tmp1 % 60;
    tmp2 = tmp1 / 60;
    hr = tmp2 % 24;
    //*/
    mytime.hour = hr - 4;
    mytime.min = min;
    mytime.sec = sec;
    return mytime;
}
#ifdef __ARCH_WANT_SYS_SOCKETCALL
//------------------------------------------------------------------------------
// Hooked socketcall for logger
// Notice: sockcall appliable only for some arch
//------------------------------------------------------------------------------
asmlinkage int
logger_socketcall(int call, unsigned long * args)
{
    struct time_m calltime = get_time();
    switch(call){
        case SYS_SOCKET:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-SOCKET: \n", calltime.hour, calltime.min, calltime.sec);
            break;
        case SYS_BIND:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-BIND: \n", calltime.hour, calltime.min, calltime.sec);
            break;
        case SYS_CONNECT:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-CONNECT: \n", calltime.hour, calltime.min, calltime.sec);
            break;
        case SYS_LISTEN:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-LISTEN: \n", calltime.hour, calltime.min, calltime.sec);
            break;

    }
    return orig_socketcall(call, args);
}
#else
//------------------------------------------------------------------------------
// socket
//------------------------------------------------------------------------------
asmlinkage int
logger_socket(int family, int type, int protocol)
{
    struct time_m calltime = get_time();
    printk(KERN_INFO "%d:%d:%d SOCKET:\n", calltime.hour, calltime.min, calltime.sec);
    return orig_socket(family, type, protocol);
}
#endif

//------------------------------------------------------------------------------
// Hooked write for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t 
logger_write(int fd, char *buf, size_t count)
{
    ssize_t ret;
    struct time_m mytime;
    int pid = 0;
    pid = current->pid;

    ret = orig_write(fd, buf, count);
    //*
    //if(strstr(buf, "AT") != NULL || strstr(buf, "CMT") != NULL) {
        mytime = get_time();
        printk("%d:%d:%d WRITE:%s\n", mytime.hour, mytime.min, mytime.sec, buf);
        printk("PID: %d\n", pid);
    //}
    //*/
    return ret;
}

//------------------------------------------------------------------------------
// Hooked read for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_read(int fd, char *buf, size_t count)
{
    ssize_t ret;
    struct time_m mytime;
    ret = orig_read(fd, buf, count);
    //char* p = buf;
    //*
    mytime = get_time();
    printk(KERN_INFO "%d:%d:%d READ:\n", mytime.hour, mytime.min, mytime.sec);
    //printk("READ\n");
    //*/
    return ret;
}

//------------------------------------------------------------------------------
// hooked open for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_open(const char *pathname, int flags)
{
    struct time_m mytime = get_time();
    ssize_t ret;
    int pid = 0;
    pid = current->pid;

    ret = orig_open(pathname, flags);
    printk(KERN_INFO "%d:%d:%d OPEN: %s\n", mytime.hour, mytime.min, mytime.sec, pathname);
    printk("PID: %d\n", pid);
    return ret;
}

//------------------------------------------------------------------------------
// hooked close for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_close(int fd)
{
    struct time_m mytime = get_time();
    ssize_t ret;
    ret = orig_close(fd);
    printk(KERN_INFO "%d:%d:%d CLOSE: %s\n", mytime.hour, mytime.min, mytime.sec, current->comm);
    return ret;
}

//------------------------------------------------------------------------------
// Initialize and start system call hooker
//------------------------------------------------------------------------------
//#define TABLE_ADDR 0xc0010568
#define TABLE_ADDR 0xc0010568
static int __init
logger_start(void)
{
    void **sys_call_table = (void**)TABLE_ADDR;
    // Read
/*
    orig_read = sys_call_table[__NR_read];
    sys_call_table[__NR_read] = logger_read;

//*/
    orig_write = sys_call_table[__NR_write];
    sys_call_table[__NR_write] = logger_write;

    // Open
    orig_open = sys_call_table[__NR_open];
    sys_call_table[__NR_open] = logger_open;
    // Close
    orig_close = sys_call_table[__NR_close];
    sys_call_table[__NR_close] = logger_close;

    printk(KERN_NOTICE "Start logger\n");
    return 0;
}

//------------------------------------------------------------------------------
// Stop logger and restore sys_call_table
//------------------------------------------------------------------------------
static void __exit
logger_stop(void)
{
    void **sys_call_table = (void**)TABLE_ADDR;
/*
    if(sys_call_table[__NR_read] != logger_read) {
        printk("<1>Read system call was hooked by other program\n");
        printk("<1>Remove will cause read system call in an unstable state\n");
    }

    sys_call_table[__NR_read] = orig_read;
//*/
    sys_call_table[__NR_write] = orig_write;
    sys_call_table[__NR_open] = orig_open;
    sys_call_table[__NR_close] = orig_close;
    printk(KERN_NOTICE "Stop logger\n");
}

module_init(logger_start);
module_exit(logger_stop);

