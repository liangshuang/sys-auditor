#ifndef SYSCALL_KLOG
#define SYSCALL_KLOG
/******************************** Exported Definitions ************************/
#define PARAM_BUF_SIZE  256

struct time_m{
    int hour;
    int min;
    int sec;
};

enum klog_type {
    MYKLOG_WRITE,
    MYKLOG_READ,
    MYKLOG_OPEN,
    MYKLOG_CLOSE,
    MYKLOG_SOCKETCALL,
    MYKLOG_SOCKET,
    MYKLOG_BIND,
    MYKLOG_CONNECT,
    MYKLOG_LISTEN,
    MYKLOG_ACCEPT,
    MYKLOG_SEND,
    MYKLOG_RECV
};

struct klog_entry {
    enum klog_type type;
    struct time_m ts;
    int pid;
    int uid;
    int param_size;
    /* size above 4+12+12=28*/
    char param[PARAM_BUF_SIZE];
};

#endif // SYSCALL_KLOG
