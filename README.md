# Vendor Reset

The goal of this project is to provide a kernel module that is capable of
resetting hardware devices into a state where they can be re-initialized or
passed through into a virtual machine (VFIO). While it would be great to have
these in the kernel as PCI quirks, some of the reset procedures are very complex
and would never be accepted as a quirk (ie AMD Vega 10).

By providing this as an out of tree kernel module, vendors will be able to
easily create pull requests to add functionality to this module, and users will
be able to easily update this module without requiring a complete kernel rebuild.

## Patching the kernel

TL;DR - No patching required.

This module has been written to use `ftrace` to hook `pci_dev_specific_reset`,
allowing it to handle device resets directly without patching the running
kernel. Simply modprobing the module is enough to enable the reset routines for
all supported hardware.

## Requirements

Ensure your kernel has the following options enabled:

```
CONFIG_FTRACE=y
CONFIG_KPROBES=y
CONFIG_PCI_QUIRKS=y
CONFIG_KALLSYMS=y
CONFIG_KALLSYMS_ALL=y
CONFIG_FUNCTION_TRACER=y
```

## Installing

This module can be installed either using the standard `make`, `make install`
pattern, or through `dkms` (recommended).

    dkms install .

Put `udev/99-vendor-reset.rules` into `/etc/udev/rules.d/99-vendor-reset.rules`

    udevadm control --reload-rules && udevadm trigger

## Usage

Either `modprobe vendor-reset` or add the device to the appropriate place to
load it at system boot, such as `/etc/modules` (Debian). Consult your
distribution's documentation as to the best way to perform this.

**NOTE: ** This module must be loaded EARLY, the default reset the kernel will
try to perform completely breaks the GPU which this module can not recover from.
Please consult your distributions documentation on how to do this, for most
however it will be as simple as adding `vendor-reset` to `/etc/modules` and
updating your initrd.

Then execute: `echo device_specific > /sys/bus/pci/devices/[REPLACE_WITH_GPU_ID]/reset_method`.

Both of these things are also accomplished by 99-vendor-reset.rules.

If successful, vendor-reset will output to dmesg and prevent the Qemu reset bug:

    qemu-system-x86_64: ../qemu-9.1.2/hw/pci/pci.c:1637: pci_irq_handler: Assertion 0 <= irq_num && irq_num < PCI_NUM_PINS' failed.

Please keep in mind that vendor-reset can only fix the bug before it occurs, not after.

## Supported Devices

| Vendor | Family | Common Name(s)
|---|---|---|
|AMD|Polaris 10| RX 470, 480, 570, 580, 590 |
|AMD|Polaris 11| RX 460, 560 |
|AMD|Polaris 12| RX 540, 550 |
|AMD|Vega 10| Vega 56/64/FE |
|AMD|Vega 20| Radeon VII |
|AMD|Vega 20| Instinct MI100 |
|AMD|Vega 20| 5700U Vega 8 |
|AMD|Navi 10| 5600XT, 5700, 5700XT |
|AMD|Navi 10| 780M Phoenix1 |
|AMD|Navi 10| 2400GE Vega 11 |
|AMD|Navi 12| Pro 5600M |
|AMD|Navi 14| Pro 5300, RX 5300, 5500XT |
|AMD|Navi 23|Radeon RX 6600/6600 XT/6600M |

## Developing

If you are a vendor intending to add support for your device to this project
please first consider two things:

1. Can you fix your hardware/firmware to reset correctly using FLR or a BUS
   reset?
2. Is the reset simple enough that it should really be a kernel pci quirk
   (see: kernel/drivers/pci/quirk.c)?

If you answer yes to either of these questions this project is not for you.

## Usage with unsupported Devices

Vendor-reset triggers only if your GPU's product ID matches the listed 
devices above. However many more devices will potentially work, if their 
product IDs are simply added in device-db.h and 99-vendor-reset.rules, to 
deploy one of the four currently implemented reset strategies (POLARIS10, 
VEGA10, VEGA20, NAVI10) in a trial and error process.

Furthermore you can find some useful explanations in the amdgpu source files:
* [amdgpu.h](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/amdgpu/amdgpu.h): enum amd_reset_method 
* [amdgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/amdgpu/amdgpu_drv.c): product ID -> chip type

NAVI10 for example makes the most sense to try for iGPUs (integrated), and
people have done this with success for various models (e.g. [Vega7/5600G](https://forum.proxmox.com/threads/amd-ryzen-5600g-igpu-code-43-error.138665/post-726791) ).


