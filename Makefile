USER  := $(shell whoami)
KVER ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVER)/build

all: build

build:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load: all
	grep -q '^vendor_reset' /proc/modules && sudo rmmod vendor_reset || true
	sudo insmod ./vendor-reset.ko $(if $(HOOK),install_hook=yes,)

.PHONY: userspace load all install
