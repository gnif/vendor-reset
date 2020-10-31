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

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/pci.h>

#include "vendor-reset-dev.h"
#include "vendor-reset.h"

#include "device-db.h"

#define VENDOR_RESET_DEVNAME "vendor_reset"

static long vendor_reset_ioctl_reset(struct file * filp, unsigned long arg)
{
  struct vendor_reset_ioctl dev;
  struct vendor_reset_device *entry = vendor_reset_devices;
  struct pci_dev * pcidev;
  int ret;

  if (copy_from_user(&dev, (void __user *)arg, sizeof(dev)))
    return -EFAULT;

  pcidev = pci_get_domain_bus_and_slot(dev.domain, dev.bus, dev.devfn);
  if (!pcidev)
    return -ENODEV;

  for(entry = vendor_reset_devices; entry->vendor; ++entry)
  {
    if (entry->vendor != pcidev->vendor)
      continue;

    if (entry->device == VENDOR_RESET_DEVICE_ALL ||
        entry->device == pcidev->device)
      break;
  }

  if (!entry->vendor)
  {
    ret = -EOPNOTSUPP;
    goto err;
  }

  ret = entry->ops->reset(pcidev);

err:
  pci_dev_put(pcidev);
  return ret;
}

static long vendor_reset_ioctl(struct file * filp, unsigned int ioctl,
    unsigned long arg)
{
  long ret;

  switch(ioctl)
  {
    case VENDOR_RESET_RESET:
      ret = vendor_reset_ioctl_reset(filp, arg);
      break;

    default:
      ret = -ENOTTY;
      break;
  }

  return ret;
}

static const struct file_operations vendor_reset_fops =
{
  .owner          = THIS_MODULE,
  .unlocked_ioctl = vendor_reset_ioctl,
#ifdef CONFIG_COMPAT
  .compat_ioctl   = vendor_reset_ioctl,
#endif
};

static struct miscdevice vendor_reset_misc =
{
  .minor = MISC_DYNAMIC_MINOR,
  .name  = VENDOR_RESET_DEVNAME,
  .fops  = &vendor_reset_fops
};

static int __init vendor_reset_init(void)
{
  return misc_register(&vendor_reset_misc);
}

static void __exit vendor_reset_exit(void)
{
  misc_deregister(&vendor_reset_misc);
}

module_init(vendor_reset_init);
module_exit(vendor_reset_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Geoffrey McRae <geoff@hostfission.com>");
