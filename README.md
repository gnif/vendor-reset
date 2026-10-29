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

### Experimental Navi48 support

Navi48 (`1002:7550`) has a separate SMU 14 mode1 reset handler. It does not
reuse the Navi10 reset sequence. The handler follows the debug-mailbox
sequence in Linux v6.16's
[smu_v14_0_2_ppt.c](https://github.com/torvalds/linux/blob/v6.16/drivers/gpu/drm/amd/pm/swsmu/smu14/smu_v14_0_2_ppt.c)
and checks PSP bootloader readiness before reporting success.

The bundled [udev rules](udev/99-vendor-reset.rules) include `1002:7550`
to load the module and select `device_specific` on Linux kernels 5.15+.

This implementation has not been validated on Navi48 hardware. A Linux
kernel build and repeated VFIO guest start/stop/reset cycles on the actual
card are required before treating it as supported. macOS cannot build or
load this Linux kernel module natively.

The handler requires valid VRAM IP-discovery data, SMU/PSP version 14.0.2
or 14.0.3, and MMIO-accessible reset registers. Missing or unsupported
discovery data and reset timeouts are reported as errors; the handler
does not fall back to Navi10 register offsets. Do not use it while the
GPU is bound to `amdgpu`.

The reset sequence and discovery parser can be tested with
`make -C tests check` using a C compiler with AddressSanitizer and
UndefinedBehaviorSanitizer. These host-side tests use simulated PCI/MMIO
access and can run on macOS; they are not a Linux kernel build or a hardware test.

## Developing

If you are a vendor intending to add support for your device to this project
please first consider two things:

1. Can you fix your hardware/firmware to reset correctly using FLR or a BUS
   reset?
2. Is the reset simple enough that it should really be a kernel pci quirk
   (see: kernel/drivers/pci/quirk.c)?

If you answer yes to either of these questions this project is not for you.
