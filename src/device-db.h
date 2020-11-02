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

static struct vendor_reset_cfg vendor_reset_devices[] =
    {
        /* AMD Vega 10 */
        {PCI_VENDOR_ID_ATI, 0x6860, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x6861, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x6862, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x6863, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x6864, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x6867, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x6868, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x6869, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x686a, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x686b, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x686c, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x686d, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x686e, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x686f, &amd_vega10_ops},
        {PCI_VENDOR_ID_ATI, 0x687f, &amd_vega10_ops},

        /* AMD Vega 20 */
        {PCI_VENDOR_ID_ATI, 0x66a0, &amd_vega20_ops},
        {PCI_VENDOR_ID_ATI, 0x66a1, &amd_vega20_ops},
        {PCI_VENDOR_ID_ATI, 0x66a2, &amd_vega20_ops},
        {PCI_VENDOR_ID_ATI, 0x66a3, &amd_vega20_ops},
        {PCI_VENDOR_ID_ATI, 0x66a4, &amd_vega20_ops},
        {PCI_VENDOR_ID_ATI, 0x66a7, &amd_vega20_ops},
        {PCI_VENDOR_ID_ATI, 0x66af, &amd_vega20_ops},

        /* AMD Navi 10 */
        {PCI_VENDOR_ID_ATI, 0x7310, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x7312, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x7318, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x7319, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x731a, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x731b, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x731e, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x731f, &amd_navi10_ops},

        /* AMD Navi 14 */
        {PCI_VENDOR_ID_ATI, 0x7340, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x7341, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x7347, &amd_navi10_ops},
        {PCI_VENDOR_ID_ATI, 0x734F, &amd_navi10_ops},

        {0}};
