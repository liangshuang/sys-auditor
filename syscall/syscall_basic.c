
#include <asm/unistd.h>             /* __ARCH_WANT_SYS_SOCKETCALL */
#include <linux/time.h>
#include <linux/cred.h>             /* For UID */
#include <linux/sched.h>            /* current */
#include "klogger.h"
#include "syscall_klog.h"
#include "klog_queue.h""
/********************************** Definitions ******************************/

/******************************** Declarations ********************************/
#define HOOK_SOCKET     1
#define SYSCALL_TBL_ADDR 0xc000eb84   /* lab goldfish */
//#define SYSCALL_TBL_ADDR 0xc000ed44     /* cis-du02 goldfish 3.4 kernel */

asmlinkage ssize_t (*orig_read)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_write)(int fd, char *buf, size_t count);
asmlinkage ssize_t (*orig_open)(const char *pathname, int flags);
asmlinkage ssize_t (*orig_close)(int fd);

#if HOOK_SOCKET
#ifdef __ARCH_WANT_SYS_SOCKETCALL
asmlinkage int (*orig_socketcall)(int call, unsigned long* args);
#else
asmlinkage int (*orig_socket)(int family, int type, int protocol);
asmlinkage int (*orig_bind)(int fd, struct sockaddr* addr, int addrlen);
asmlinkage int (*orig_connect)(int fd, struct sockaddr *addr, int addrlen);
asmlinkage int (*orig_listen)(int fd, int backlog);
asmlinkage int (*orig_accept)(int fd, struct sockaddr *addr, int *addrlen);
asmlinkage ssize_t (*orig_send)(int fd, void *buf, size_t len, unsigned flags);
asmlinkage int (*orig_sendto)(int fd, void *buff, size_t len, unsigned flags, \
    struct sockaddr *addr, int addr_len);
asmlinkage ssize_t (*orig_recv)(int fd, void *buf, size_t size, unsigned flags);
asmlinkage int (*orig_recvfrom)(int fd, void *ubuf, size_t size, unsigned flags, \
    struct sockaddr *addr, int addr_len);
#endif//__ARCH_WANT_SYS_SOCKETCALL
#endif // HOOK_SOCKET

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
    memset(&e, 0, sizeof(struct klog_entry));
    e.type = type;
    e.ts = get_time();
    e.pid = current->pid;
    e.uid = current_uid();
    /*
    logfifo_write(&e, sizeof(e)); 

    if(param_size > 0)
        logfifo_write(param, param_size);
    */
    if(param_size > 0) {
        if(param_size > PARAM_BUF_SIZE)
            param_size = PARAM_BUF_SIZE;
        /* Set tuned size here, otherwise 0 */
        e.param_size = param_size;
        memcpy(e.param, param, param_size);
    }
    /* Enqueue this log entity to klog buffer */
    klog_enqueue(&e);

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
#if HOOK_SOCKET
#ifdef __ARCH_WANT_SYS_SOCKETCALL
//------------------------------------------------------------------------------
// Hooked socketcall for logger
// Notice: sockcall appliable only for some arch
//------------------------------------------------------------------------------
asmlinkage int
hooked_socketcall(int call, unsigned long * args)
{
    int ret;
    ret = orig_socketcall(call, args);
    /*
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
    //*/
    add_log_entry(MYKLOG_SOCKETCALL, NULL, 0);
    return ret;
}
#else
//------------------------------------------------------------------------------
// socket
//------------------------------------------------------------------------------
asmlinkage int
hooked_socket(int family, int type, int protocol)
{
    int ret;
    ret = orig_socket(family, type, protocol);
    // Add log entry
    add_log_entry(MYKLOG_SOCKET, NULL, 0);
    return ret;
}

//------------------------------------------------------------------------------
// bind
//------------------------------------------------------------------------------
asmlinkage int
hooked_bind(int fd, struct sockaddr* addr, int addrlen)
{
    int ret;
    ret = orig_bind(fd, addr, addrlen);
    add_log_entry(MYKLOG_BIND, NULL, 0);
    return ret;
}
//------------------------------------------------------------------------------
// connect
//------------------------------------------------------------------------------
asmlinkage int
hooked_connect(int fd, struct sockaddr *addr, int addrlen)
{
    int ret;
    ret = orig_connect(fd, addr, addrlen);
    add_log_entry(MYKLOG_CONNECT, NULL, 0);
    return ret;

}
//------------------------------------------------------------------------------
// listen
//------------------------------------------------------------------------------
asmlinkage int
hooked_listen(int fd, int backlog)
{
    int ret;
    ret = orig_listen(fd, backlog);
    add_log_entry(MYKLOG_LISTEN, NULL, 0);
    return ret;
}
//------------------------------------------------------------------------------
// accept
//------------------------------------------------------------------------------
asmlinkage int
hooked_accept(int fd, struct sockaddr *addr, int *addrlen)
{
    int ret;
    ret = orig_accept(fd, addr, addrlen);
    add_log_entry(MYKLOG_ACCEPT, NULL, 0);
    return ret;
}
//------------------------------------------------------------------------------
// send
//------------------------------------------------------------------------------
asmlinkage ssize_t
hooked_send(int fd, void *buf, size_t len, unsigned flags)
{
    int ret;
    ret = orig_send(fd, buf, len, flags);
    add_log_entry(MYKLOG_SEND, buf, len);
    return ret;
}

asmlinkage ssize_t hooked_recv(int fd, void *buf, size_t size, unsigned flags)
{
    ssize_t ret;
    ret = orig_recv(fd, buf, size, flags);
    add_log_entry(MYKLOG_RECV, buf, size);
    return ret;
}

asmlinkage int hooked_recvfrom(int fd, void *ubuf, size_t size, unsigned flags, \
    struct sockaddr *addr, int addr_len)
{
    int ret;
    ret = orig_recvfrom(fd, ubuf, size, flags, addr, addr_len);
    add_log_entry(MYKLOG_RECVFROM, ubuf, size);
    return ret;
}

asmlinkage int hooked_sendto(int fd, void *buff, size_t len, unsigned flags, \
    struct sockaddr *addr, int addr_len)
{
    int ret;
    ret = orig_sendto(fd, buff, len, flags, addr, addr_len);
    add_log_entry(MYKLOG_SENDTO, buff, len);
    return ret;
}

#endif
#endif //HOOK_SOCKET


//------------------------------------------------------------------------------
// Initialize and start system call hooker
//------------------------------------------------------------------------------
int __init hook_start(void)
{
    /* Start system call hooks */
    void **sys_call_table = (void**)SYSCALL_TBL_ADDR;
    printk(KERN_NOTICE "Install hooker...\n");
    // Read
//* TODO: Causes system reboot when rmmod with read 
    orig_read = sys_call_table[__NR_read];
    sys_call_table[__NR_read] = hooked_read;

//*/
    orig_write = sys_call_table[__NR_write];
    sys_call_table[__NR_write] = hooked_write;

    // Open
    orig_open = sys_call_table[__NR_open];
    sys_call_table[__NR_open] = hooked_open;
    //*
    // Close
    orig_close = sys_call_table[__NR_close];
    sys_call_table[__NR_close] = hooked_close;
//*/
    /* Sockets */
#if HOOK_SOCKET
#ifdef __ARCH_WANT_SYS_SOCKETCALL
    orig_socketcall = sys_call_table[__NR_socketcall];
    sys_call_table[__NR_socketcall] = hooked_socketcall;
#else
    orig_socket = sys_call_table[__NR_socket];
    sys_call_table[__NR_socket] = hooked_socket;

    orig_bind = sys_call_table[__NR_bind];
    sys_call_table[__NR_bind] = hooked_bind;

    orig_connect = sys_call_table[__NR_connect];
    sys_call_table[__NR_connect] = hooked_connect;

    orig_listen = sys_call_table[__NR_listen];
    sys_call_table[__NR_listen] = hooked_listen;

    orig_accept = sys_call_table[__NR_accept];
    sys_call_table[__NR_accept] = hooked_accept;

    orig_send = sys_call_table[__NR_send];
    sys_call_table[__NR_send] = hooked_send;

    orig_sendto = sys_call_table[__NR_sendto];
    sys_call_table[__NR_sendto] = hooked_sendto;

    orig_recv = sys_call_table[__NR_recv];
    sys_call_table[__NR_recv] = hooked_recv;

    orig_recvfrom = sys_call_table[__NR_recvfrom];
    sys_call_table[__NR_recvfrom] = hooked_recvfrom;
#endif
#endif // HOOK_SOCKET
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

//*/
    sys_call_table[__NR_read] = orig_read;
    sys_call_table[__NR_write] = orig_write;
    sys_call_table[__NR_open] = orig_open;
    //*
    sys_call_table[__NR_close] = orig_close;
    //*/
#if HOOK_SOCKET
#ifdef __ARCH_WANT_SYS_SOCKETCALL
    sys_call_table[__NR_socketcall] = orig_socketcall;
#else
    sys_call_table[__NR_socket] = orig_socket;
    sys_call_table[__NR_bind] = orig_bind;
    sys_call_table[__NR_connect] = orig_connect;
    sys_call_table[__NR_listen] = orig_listen;
    sys_call_table[__NR_accept] = orig_accept;
    sys_call_table[__NR_send] = orig_send;
    sys_call_table[__NR_sendto] = orig_sendto;
    sys_call_table[__NR_recv] == orig_recv;
    sys_call_table[__NR_recvfrom] = orig_recvfrom;
#endif 
#endif // HOOK_SOCKET
}

