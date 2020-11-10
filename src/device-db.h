/*
Vendor Reset - Vendor Specific Reset
Copyright (C) 2020 Geoffrey McRae <geoff@hostfission.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "amd/amd.h"

#define AMD_POLARIS10(op)            \
    {PCI_VENDOR_ID_ATI, 0x67C0, op}, \
    {PCI_VENDOR_ID_ATI, 0x67C1, op}, \
    {PCI_VENDOR_ID_ATI, 0x67C2, op}, \
    {PCI_VENDOR_ID_ATI, 0x67C4, op}, \
    {PCI_VENDOR_ID_ATI, 0x67C7, op}, \
    {PCI_VENDOR_ID_ATI, 0x67D0, op}, \
    {PCI_VENDOR_ID_ATI, 0x67DF, op}, \
    {PCI_VENDOR_ID_ATI, 0x67C8, op}, \
    {PCI_VENDOR_ID_ATI, 0x67C9, op}, \
    {PCI_VENDOR_ID_ATI, 0x67CA, op}, \
    {PCI_VENDOR_ID_ATI, 0x67CC, op}, \
    {PCI_VENDOR_ID_ATI, 0x67CF, op}, \
    {PCI_VENDOR_ID_ATI, 0x6FDF, op}

#define AMD_VEGA10(op)               \
    {PCI_VENDOR_ID_ATI, 0x6860, op}, \
    {PCI_VENDOR_ID_ATI, 0x6861, op}, \
    {PCI_VENDOR_ID_ATI, 0x6862, op}, \
    {PCI_VENDOR_ID_ATI, 0x6863, op}, \
    {PCI_VENDOR_ID_ATI, 0x6864, op}, \
    {PCI_VENDOR_ID_ATI, 0x6867, op}, \
    {PCI_VENDOR_ID_ATI, 0x6868, op}, \
    {PCI_VENDOR_ID_ATI, 0x6869, op}, \
    {PCI_VENDOR_ID_ATI, 0x686a, op}, \
    {PCI_VENDOR_ID_ATI, 0x686b, op}, \
    {PCI_VENDOR_ID_ATI, 0x686c, op}, \
    {PCI_VENDOR_ID_ATI, 0x686d, op}, \
    {PCI_VENDOR_ID_ATI, 0x686e, op}, \
    {PCI_VENDOR_ID_ATI, 0x686f, op}, \
    {PCI_VENDOR_ID_ATI, 0x687f, op}

#define AMD_VEGA20(op)               \
    {PCI_VENDOR_ID_ATI, 0x66a0, op}, \
    {PCI_VENDOR_ID_ATI, 0x66a1, op}, \
    {PCI_VENDOR_ID_ATI, 0x66a2, op}, \
    {PCI_VENDOR_ID_ATI, 0x66a3, op}, \
    {PCI_VENDOR_ID_ATI, 0x66a4, op}, \
    {PCI_VENDOR_ID_ATI, 0x66a7, op}, \
    {PCI_VENDOR_ID_ATI, 0x66af, op}

#define AMD_NAVI10(op)               \
    {PCI_VENDOR_ID_ATI, 0x7310, op}, \
    {PCI_VENDOR_ID_ATI, 0x7312, op}, \
    {PCI_VENDOR_ID_ATI, 0x7318, op}, \
    {PCI_VENDOR_ID_ATI, 0x7319, op}, \
    {PCI_VENDOR_ID_ATI, 0x731a, op}, \
    {PCI_VENDOR_ID_ATI, 0x731b, op}, \
    {PCI_VENDOR_ID_ATI, 0x731e, op}, \
    {PCI_VENDOR_ID_ATI, 0x731f, op}

#define AMD_NAVI14(op)               \
    {PCI_VENDOR_ID_ATI, 0x7340, op}, \
    {PCI_VENDOR_ID_ATI, 0x7341, op}, \
    {PCI_VENDOR_ID_ATI, 0x7347, op}, \
    {PCI_VENDOR_ID_ATI, 0x734F, op}

static struct vendor_reset_cfg vendor_reset_devices[] = {
    AMD_POLARIS10(&amd_polaris10_ops),
    AMD_VEGA10(&amd_vega10_ops),
    AMD_VEGA20(&amd_vega20_ops),
    AMD_NAVI10(&amd_navi10_ops),
    AMD_NAVI14(&amd_navi10_ops),
    {0},
};
