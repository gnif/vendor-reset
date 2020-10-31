obj-m += vendor-reset.o
USER  := $(shell whoami)
KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

userspace:
	gcc userspace/vendor-reset.c -Wall -Werror -g -Og -o userspace/vendor-reset

load: all
	grep -q '^vendor-reset' /proc/modules && sudo rmmod vendor-reset || true
	sudo insmod ./vendor-reset.ko

.PHONY: userspace
