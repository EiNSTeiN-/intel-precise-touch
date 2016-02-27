#
#  pacman -S linux-headers

# file : Makefile
# desc : Build linux device driver and userspace sources for
#         test with the QEMU ivshmem PCI device
#

krn = mssp4touch

ifneq ($(KERNELRELEASE),)

obj-m := $(krn).o

else

#KDIR ?= /lib/modules/$$(uname -r)/build
KDIR ?= /lib/modules/4.3.3-3-ARCH/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules
	#$(CC) -Wall -O2 $(usr).c -o $(usr)

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
	rm -f $(usr)

.PHONY : clean
endif
