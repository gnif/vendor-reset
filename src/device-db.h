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

#define _AMD_POLARIS10(op)                          \
    {PCI_VENDOR_ID_ATI, 0x67C0, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67C1, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67C2, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67C4, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67C7, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67D0, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67DF, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67C8, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67C9, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67CA, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67CC, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x67CF, op, AMD_POLARIS10}, \
    {PCI_VENDOR_ID_ATI, 0x6FDF, op, AMD_POLARIS10}

#define _AMD_POLARIS11(op)                          \
    {PCI_VENDOR_ID_ATI, 0x67E0, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67E3, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67E8, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67EB, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67EF, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67FF, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67E1, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67E7, op, AMD_POLARIS11}, \
    {PCI_VENDOR_ID_ATI, 0x67E9, op, AMD_POLARIS11}

#define _AMD_POLARIS12(op)                          \
    {PCI_VENDOR_ID_ATI, 0x6980, op, AMD_POLARIS12}, \
    {PCI_VENDOR_ID_ATI, 0x6981, op, AMD_POLARIS12}, \
    {PCI_VENDOR_ID_ATI, 0x6985, op, AMD_POLARIS12}, \
    {PCI_VENDOR_ID_ATI, 0x6986, op, AMD_POLARIS12}, \
    {PCI_VENDOR_ID_ATI, 0x6987, op, AMD_POLARIS12}, \
    {PCI_VENDOR_ID_ATI, 0x6995, op, AMD_POLARIS12}, \
    {PCI_VENDOR_ID_ATI, 0x6997, op, AMD_POLARIS12}, \
    {PCI_VENDOR_ID_ATI, 0x699F, op, AMD_POLARIS12}

#define _AMD_VEGA10(op)                          \
    {PCI_VENDOR_ID_ATI, 0x6860, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x6861, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x6862, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x6863, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x6864, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x6867, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x6868, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x6869, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x686a, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x686b, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x686c, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x686d, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x686e, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x686f, op, AMD_VEGA10}, \
    {PCI_VENDOR_ID_ATI, 0x687f, op, AMD_VEGA10}

#define _AMD_VEGA20(op)                          \
    {PCI_VENDOR_ID_ATI, 0x66a0, op, AMD_VEGA20}, \
    {PCI_VENDOR_ID_ATI, 0x66a1, op, AMD_VEGA20}, \
    {PCI_VENDOR_ID_ATI, 0x66a2, op, AMD_VEGA20}, \
    {PCI_VENDOR_ID_ATI, 0x66a3, op, AMD_VEGA20}, \
    {PCI_VENDOR_ID_ATI, 0x66a4, op, AMD_VEGA20}, \
    {PCI_VENDOR_ID_ATI, 0x66a7, op, AMD_VEGA20}, \
    {PCI_VENDOR_ID_ATI, 0x66af, op, AMD_VEGA20}

#define _AMD_NAVI10(op)                          \
    {PCI_VENDOR_ID_ATI, 0x7310, op, AMD_NAVI10}, \
    {PCI_VENDOR_ID_ATI, 0x7312, op, AMD_NAVI10}, \
    {PCI_VENDOR_ID_ATI, 0x7318, op, AMD_NAVI10}, \
    {PCI_VENDOR_ID_ATI, 0x7319, op, AMD_NAVI10}, \
    {PCI_VENDOR_ID_ATI, 0x731a, op, AMD_NAVI10}, \
    {PCI_VENDOR_ID_ATI, 0x731b, op, AMD_NAVI10}, \
    {PCI_VENDOR_ID_ATI, 0x731e, op, AMD_NAVI10}, \
    {PCI_VENDOR_ID_ATI, 0x731f, op, AMD_NAVI10}

#define _AMD_NAVI14(op)                          \
    {PCI_VENDOR_ID_ATI, 0x7340, op, AMD_NAVI14}, \
    {PCI_VENDOR_ID_ATI, 0x7341, op, AMD_NAVI14}, \
    {PCI_VENDOR_ID_ATI, 0x7347, op, AMD_NAVI14}, \
    {PCI_VENDOR_ID_ATI, 0x734F, op, AMD_NAVI14}

#define _AMD_NAVI12(op)                          \
    {PCI_VENDOR_ID_ATI, 0x7360, op, AMD_NAVI12}, \
    {PCI_VENDOR_ID_ATI, 0x7362, op, AMD_NAVI12}

static struct vendor_reset_cfg vendor_reset_devices[] = {
    _AMD_POLARIS10(&amd_polaris10_ops),
    _AMD_POLARIS11(&amd_polaris10_ops),
    _AMD_POLARIS12(&amd_polaris10_ops),
    _AMD_VEGA10(&amd_vega10_ops),
    _AMD_VEGA20(&amd_vega20_ops),
    _AMD_NAVI10(&amd_navi10_ops),
    _AMD_NAVI14(&amd_navi10_ops),
    _AMD_NAVI12(&amd_navi10_ops),
    {0},
};
