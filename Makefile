USER  := $(shell whoami)
KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: all
	grep -q '^vendor_reset' /proc/modules && sudo rmmod vendor_reset || true
	sudo insmod ./vendor-reset.ko

.PHONY: userspace load
