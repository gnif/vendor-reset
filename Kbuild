obj-m += vendor-reset.o

include $(src)/src/Makefile
include $(src)/src/amd/Makefile

ccflags-y += \
  -I$(src)/include -g
ldflags-$(CONFIG_DEBUG) += -g

#subdir-y += userspace/
