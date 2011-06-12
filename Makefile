ifneq ($(KERNELRELEASE),)
	obj-m := usbtinyisp.o
	usbtinyisp-objs := usbtiny.o controls.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif

