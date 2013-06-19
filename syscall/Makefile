obj-m += klogger.o
klogger-objs := klog_queue.o syscall_basic.o klogger_basic.o

#KERNEL_DIR=~/Working/workspace/android/kernels/goldfish/
KERNEL_DIR=~/Working/workspace/android/kernels/lge/android-msm-mako-3.4-jb-mr1.1

PWD := $(shell pwd)

all:
	make CFLAGS_MODULE=-fno-pic -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
