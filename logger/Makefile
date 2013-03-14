obj-m += syscall_logger.o
KERNEL_DIR=~/Working/workspace/android/kernels/goldfish/
PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
