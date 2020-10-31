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

#ifndef _H_VENDOR_RESET_IOCTL
#define _H_VENDOR_RESET_IOCTL

#include <linux/types.h>
#include <linux/ioctl.h>

#define VENDOR_RESET_MAGIC 'r'

#define VENDOR_RESET_TYPE_PCI 0x1
#define VENDOR_RESET_TYPE_USB 0x2

struct vendor_reset_ioctl
{
  int type;
  int domain;
  int bus;
  int devfn;
};

#define VENDOR_RESET_RESET _IOW(VENDOR_RESET_MAGIC, 1, struct vendor_reset_ioctl)

#endif
