obj-m += syscall_logger.o
#KERNEL_DIR=~/Working/workspace/android/kernels/goldfish/
KERNEL_DIR=~/Working/workspace/android/kernels/lge/msm

PWD := $(shell pwd)

all:
	make CFLAGS_MODULE=-fno-pic -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
