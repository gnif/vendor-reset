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

## Usage

Either `modprobe vendor-reset` or add the device to the appropriate place to
load it at system boot, such as `/etc/modules` (Debian). Consult your
distribution's documentation as to the best way to perform this.

**NOTE: ** This module must be loaded EARLY, the default reset the kernel will
try to perform completely breaks the GPU which this module can not recover from.
Please consult your distributions documentation on how to do this, for most
however it will be as simple as adding `vendor-reset` to `/etc/modules` and
updating your initrd.

## Supported Devices

| Vendor | Family | Common Name(s)
|---|---|---|
|AMD|Polaris 10| RX 470, 480, 570, 580, 590
|AMD|Polaris 11| RX 460, 560
|AMD|Polaris 12| RX 540, 550
|AMD|Vega 10| Vega 56/64/FE |
|AMD|Vega 20| Radeon VII |
|AMD|Vega 20| Instinct MI100 |
|AMD|Navi 10| 5600XT, 5700, 5700XT
|AMD|Navi 10| 680M Rembrandt |
|AMD|Navi 12| Pro 5600M |
|AMD|Navi 14| Pro 5300, RX 5300, 5500XT

## Developing

If you are a vendor intending to add support for your device to this project
please first consider two things:

1. Can you fix your hardware/firmware to reset correctly using FLR or a BUS
   reset?
2. Is the reset simple enough that it should really be a kernel pci quirk
   (see: kernel/drivers/pci/quirk.c)?

If you answer yes to either of these questions this project is not for you.

