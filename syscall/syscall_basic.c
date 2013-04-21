
#include <asm/unistd.h>             /* __ARCH_WANT_SYS_SOCKETCALL */
#include <linux/time.h>
#include <linux/cred.h>             /* For UID */
#include <linux/sched.h>            /* current */
#include "klogger.h"
#include "syscall_klog.h"
/********************************** Definitions ******************************/

/******************************** Declarations ********************************/
#define SYSCALL_TBL_ADDR 0xc000eb84

asmlinkage ssize_t (*orig_read)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_write)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_open)(const char *pathname, int flags);
asmlinkage ssize_t (*orig_close)(int fd);

asmlinkage int (*orig_socketcall)(int call, unsigned long *args);
asmlinkage int (*orig_socket)(int family, int type, int protocol);

/********************************* Function Entry ******************************/

//------------------------------------------------------------------------------
// Get current local time and stored hour, min, secend in time_m structure
//------------------------------------------------------------------------------
struct time_m get_time(void)
{
    unsigned long time_secs;
    int sec, hr, min, tmp1,tmp2;
    struct timeval tv;
    struct time_m callTime;

    do_gettimeofday(&tv);
    //*
    time_secs = tv.tv_sec;
    sec = time_secs % 60;
    tmp1 = time_secs / 60;
    min = tmp1 % 60;
    tmp2 = tmp1 / 60;
    hr = tmp2 % 24;
    //*/
    callTime.hour = hr - 4;
    callTime.min = min;
    callTime.sec = sec;
    return callTime;
}
inline static void add_log_entry(enum klog_type type, char* param, int param_size)
{
    struct klog_entry e;

    e.type = type;
    e.ts = get_time();
    e.pid = current->pid;
    e.uid = current_uid();
    e.param_size = param_size;
    logfifo_write(&e, sizeof(e)); 

    if(param_size > 0)
        logfifo_write(param, param_size);

}
//------------------------------------------------------------------------------
// Hooked write for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t 
hooked_write(int fd, char *buf, size_t count)
{
    ssize_t ret;
    ret = orig_write(fd, buf, count);
    add_log_entry(MYKLOG_WRITE, buf, count);
    return ret;
}

//------------------------------------------------------------------------------
// Hooked read for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
hooked_read(int fd, char *buf, size_t count)
{
    ssize_t ret;
    ret = orig_read(fd, buf, count);
    add_log_entry(MYKLOG_READ, buf, count);
    return ret;
}

//------------------------------------------------------------------------------
// hooked open for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
hooked_open(const char *pathname, int flags)
{
    struct time_m callTime = get_time();
    ssize_t ret;
    ret = orig_open(pathname, flags);
    add_log_entry(MYKLOG_OPEN, pathname, strlen(pathname));
    return ret;
}

//------------------------------------------------------------------------------
// hooked close for logger
//------------------------------------------------------------------------------
asmlinkage ssize_t
hooked_close(int fd)
{
    ssize_t ret;
    ret = orig_close(fd);
    /* Add log entry */
    add_log_entry(MYKLOG_CLOSE, NULL, 0);
    return ret;
}

#ifdef __ARCH_WANT_SYS_SOCKETCALL
//------------------------------------------------------------------------------
// Hooked socketcall for logger
// Notice: sockcall appliable only for some arch
//------------------------------------------------------------------------------
asmlinkage int
hooked_socketcall(int call, unsigned long * args)
{
    struct time_m callTime = get_time();
    switch(call){
        case SYS_SOCKET:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-SOCKET: \n", callTime.hour, callTime.min, callTime.sec);
            break;
        case SYS_BIND:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-BIND: \n", callTime.hour, callTime.min, callTime.sec);
            break;
        case SYS_CONNECT:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-CONNECT: \n", callTime.hour, callTime.min, callTime.sec);
            break;
        case SYS_LISTEN:
            printk(KERN_INFO "%d:%d:%d SOCKETCALL-LISTEN: \n", callTime.hour, callTime.min, callTime.sec);
            break;
    }
    return orig_socketcall(call, args);
}
#else
//------------------------------------------------------------------------------
// socket
//------------------------------------------------------------------------------
asmlinkage int
hooked_socket(int family, int type, int protocol)
{
    struct time_m callTime = get_time();
    printk(KERN_INFO "%d:%d:%d SOCKET:\n", callTime.hour, callTime.min, callTime.sec);
    return orig_socket(family, type, protocol);
}
#endif



//------------------------------------------------------------------------------
// Initialize and start system call hooker
//------------------------------------------------------------------------------
int __init hook_start(void)
{
    /* Start system call hooks */
    void **sys_call_table = (void**)SYSCALL_TBL_ADDR;
    printk(KERN_NOTICE "Install hooker...\n");
    // Read
/*
    orig_read = sys_call_table[__NR_read];
    sys_call_table[__NR_read] = hooked_read;

    orig_write = sys_call_table[__NR_write];
    sys_call_table[__NR_write] = hooked_write;

//*/
    // Open
    orig_open = sys_call_table[__NR_open];
    sys_call_table[__NR_open] = hooked_open;
    /*
    // Close
    orig_close = sys_call_table[__NR_close];
    sys_call_table[__NR_close] = hooked_close;
//*/
    return 0;
}

//------------------------------------------------------------------------------
// Stop logger and restore sys_call_table
//------------------------------------------------------------------------------
void __exit hook_stop(void)
{
    void **sys_call_table = (void**)SYSCALL_TBL_ADDR;
    printk(KERN_NOTICE "Remove hooker\n");
/*
    if(sys_call_table[__NR_read] != hooked_read) {
        printk("<1>Read system call was hooked by other program\n");
        printk("<1>Remove will cause read system call in an unstable state\n");
    }

    sys_call_table[__NR_read] = orig_read;
    sys_call_table[__NR_write] = orig_write;
//*/
    sys_call_table[__NR_open] = orig_open;
    /*
    sys_call_table[__NR_close] = orig_close;
    */
}
