#ifndef KLOGGER_H
#define KLOGGER_H

/*********************************** Exported Functions ***********************/
ssize_t logfifo_write(const char *addr, size_t count);
void __exit hook_stop(void);
int __init hook_start(void);

#endif // KLOGGER_H
