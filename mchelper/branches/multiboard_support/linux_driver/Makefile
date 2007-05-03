ifneq ($(KERNELRELEASE),)
obj-m	:= at91.o
else
KDIR	:= /lib/modules/$(shell uname -r)/build
PWD		:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
endif

modules: at91.o