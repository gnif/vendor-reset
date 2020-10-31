obj-m          := vendor-reset.o
vendor-reset-y := src/vendor-reset.o

vendor-reset-y += src/amd/vega10.o
vendor-reset-y += src/amd/vega20.o
vendor-reset-y += src/amd/navi10.o


ccflags-y := -I$(src)/src
ccflags-y += -I$(src)/include

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
