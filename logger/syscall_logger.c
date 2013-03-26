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
#include <linux/version.h>
#include <linux/workqueue.h>

#include <linux/time.h>
#include <linux/net.h>
#include <linux/socket.h>

asmlinkage ssize_t (*orig_read)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_write)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_open)(const char *pathname, int flags);
asmlinkage ssize_t (*orig_close)(int fd);
#ifdef __ARCH_WANT_SYS_SOCKETCALL
asmlinkage int (*orig_socketcall)(int call, unsigned long* args);
#else
asmlinkage int (*orig_socket)(int family, int type, int protocol);
asmlinkage int (*orig_bind)(int fd, struct sockaddr* addr, int addrlen);
asmlinkage int (*orig_connect)(int fd, struct sockaddr *addr, int addrlen);
asmlinkage int (*orig_listen)(int fd, int backlog);
asmlinkage int (*orig_accept)(int fd, struct sockaddr *addr, int *addrlen);
asmlinkage ssize_t (*orig_send)(int fd, void *buf, size_t len, unsigned flags);
#endif

struct time_m{
    int hour;
    int min;
    int sec;
};

struct time_m get_time()
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
//------------------------------------------------------------------------------
// Hooked write for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t 
logger_write(int fd, char *buf, size_t count)
{
    //if(strstr(buf, "AT") != NULL || strstr(buf, "CMT") != NULL) {
        struct time_m mytime = get_time();
        printk(KERN_INFO "%d:%d:%d WRITE: %s\n", mytime.hour, \
                mytime.min, mytime.sec, buf);
    //}
    return orig_write(fd, buf, count);
}

//------------------------------------------------------------------------------
// Hooked read for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_read(int fd, char *buf, size_t count)
{
    char* p = buf;
    struct time_m mytime = get_time();
    printk(KERN_INFO "%d:%d:%d READ: %s", mytime.hour, mytime.min, mytime.sec, buf);
    return orig_read(fd, buf, count);
}

//------------------------------------------------------------------------------
// hooked open for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_open(const char *pathname, int flags)
{
    struct time_m mytime = get_time();
    //printk(KERN_INFO "%d:%d:%d OPEN: %s\n", mytime.hour, mytime.min, mytime.sec, pathname);
    return orig_open(pathname, flags);
}

//------------------------------------------------------------------------------
// hooked close for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_close(int fd)
{
    struct time_m mytime = get_time();
    //printk(KERN_INFO "%d:%d:%d CLOSE: %s\n", mytime.hour, mytime.min, mytime.sec, current->comm);
    return orig_close(fd);
}
#ifdef __ARCH_WANT_SYS_SOCKETCALL
//------------------------------------------------------------------------------
// hooked socketcall for logger
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
//------------------------------------------------------------------------------
// bind
//------------------------------------------------------------------------------
asmlinkage int
logger_bind(int fd, struct sockaddr* addr, int addrlen)
{
    struct time_m calltime = get_time();
    printk("%d:%d:%d BIND: \n", calltime.hour, calltime.min, calltime.sec);
    return orig_bind(fd, addr, addrlen);
}
//------------------------------------------------------------------------------
// connect
//------------------------------------------------------------------------------
asmlinkage int
logger_connect(int fd, struct sockaddr *addr, int addrlen)
{
    struct time_m calltime = get_time();
    printk("%d:%d:%d CONNECT: \n", calltime.hour, calltime.min, calltime.sec);
    return orig_connect(fd, addr, addrlen);
}
//------------------------------------------------------------------------------
// listen
//------------------------------------------------------------------------------
asmlinkage int
logger_listen(int fd, int backlog)
{
    struct time_m calltime = get_time();
    printk("%d:%d:%d LISTEN: \n", calltime.hour, calltime.min, calltime.sec);
    return orig_listen(fd, backlog);
}
//------------------------------------------------------------------------------
// accept
//------------------------------------------------------------------------------
asmlinkage int
logger_accept(int fd, struct sockaddr *addr, int *addrlen)
{
    struct time_m calltime = get_time();
    printk("%d:%d:%d ACCEPT: \n", calltime.hour, calltime.min, calltime.sec);
    return orig_accept(fd, addr, addrlen);
}
//------------------------------------------------------------------------------
// send
//------------------------------------------------------------------------------
asmlinkage ssize_t
logger_send(int fd, void *buf, size_t len, unsigned flags)
{
    struct time_m calltime = get_time();
    printk("%d:%d:%d SEND: \n", calltime.hour, calltime.min, calltime.sec);
    return orig_send(fd, buf, len, flags);
}

#endif  // __ARCH_WANT_SYS_SOCKETCALL

//------------------------------------------------------------------------------
// Initialize and start system call hooker
//------------------------------------------------------------------------------
static int __init
logger_start(void)
{
    unsigned long *sys_call_table = (unsigned long*)0xc0027f84;

    /*
    orig_read = sys_call_table[__NR_read];
    sys_call_table[__NR_read] = logger_read;
    orig_write = sys_call_table[__NR_write];
    sys_call_table[__NR_write] = logger_write;
    orig_open = sys_call_table[__NR_open];
    sys_call_table[__NR_open] = logger_open;
    orig_close = sys_call_table[__NR_close];
    sys_call_table[__NR_close] = logger_close;
    */
#ifdef __ARCH_WANT_SYS_SOCKETCALL
    orig_socketcall = sys_call_table[__NR_socketcall];
    sys_call_table[__NR_socketcall] = logger_socketcall;
#else
    orig_socket = sys_call_table[__NR_socket];
    sys_call_table[__NR_socket] = logger_socket;

    orig_bind = sys_call_table[__NR_bind];
    sys_call_table[__NR_bind] = logger_bind;

    orig_connect = sys_call_table[__NR_connect];
    sys_call_table[__NR_connect] = logger_connect;

    orig_listen = sys_call_table[__NR_listen];
    sys_call_table[__NR_listen] = logger_listen;

    orig_accept = sys_call_table[__NR_accept];
    sys_call_table[__NR_accept] = logger_accept;

    orig_send = sys_call_table[__NR_send];
    sys_call_table[__NR_send] = logger_send;
#endif
    printk(KERN_NOTICE "Start logger\n");
    return 0;
}

//------------------------------------------------------------------------------
// Stop logger and restore sys_call_table
//------------------------------------------------------------------------------
static void __exit
logger_stop(void)
{
    unsigned long *sys_call_table = (unsigned long*)0xc0027f84;
    /*
    sys_call_table[__NR_read] = orig_read;
    sys_call_table[__NR_write] = orig_write;
    sys_call_table[__NR_open] = orig_open;
    sys_call_table[__NR_close] = orig_close;
    */
#ifdef __ARCH_WANT_SYS_SOCKETCALL
    sys_call_table[__NR_socketcall] = orig_socketcall;
#else
    sys_call_table[__NR_socket] = orig_socket;
    sys_call_table[__NR_bind] = orig_bind;
    sys_call_table[__NR_connect] = orig_connect;
    sys_call_table[__NR_listen] = orig_listen;
    sys_call_table[__NR_accept] = orig_accept;
    sys_call_table[__NR_send] = orig_send;
#endif 
    printk(KERN_NOTICE "Stop logger");
}

module_init(logger_start);
module_exit(logger_stop);

