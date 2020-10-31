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

#ifndef _H_VENDOR_RESET
#define _H_VENDOR_RESET

#define VENDOR_RESET_TYPE_INVALID 0x0
#define VENDOR_RESET_TYPE_PCI     0x1

#define VENDOR_RESET_DEVICE_ALL ((unsigned int)-1)

#include <linux/pci.h>

struct vendor_reset_ops
{
  /* the reset method for the device at the specified address */
  int (*reset)(struct pci_dev *dev);
};

struct vendor_reset_device
{
  /* one of VENDOR_RESET_TYPE_* */
  unsigned int type;

  /* the vendor ID */
  unsigned int vendor;

  /* the device ID or VENDOR_RESET_DEVICE_ALL to match all devices for the
   * vendor */
  unsigned int device;

  /* the reset operations */
  struct vendor_reset_ops ops;
};

#endif
