#ifndef SYSCALL_KLOG
#define SYSCALL_KLOG
/******************************** Exported Definitions ************************/
#define PARAM_BUF_SIZE  128

struct time_m{
    int hour;
    int min;
    int sec;
};

enum klog_type {
    MYKLOG_WRITE,
    MYKLOG_READ,
    MYKLOG_OPEN,
    MYKLOG_CLOSE
};

struct klog_entry {
    enum klog_type type;
    struct time_m ts;
    int pid;
    int uid;
    int param_size;
    char param[PARAM_BUF_SIZE];
};

#endif // SYSCALL_KLOG
