/*
Vendor Reset - Vendor Specific Reset
Copyright (C) 2020 Geoffrey McRae <geoff@hostfission.com>
Copyright (C) 2020 Adam Madsen <adam@ajmadsen.com>

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

#include "vendor-reset-dev.h"
#include "vendor-reset-ioctl.h"

#include <linux/miscdevice.h>

#define VENDOR_RESET_IOCTL_DEVNAME "vendor_reset"

static long vendor_reset_ioctl_reset(struct file * filp, unsigned long arg)
{
  struct vendor_reset_ioctl iodev;
  const struct vendor_reset_cfg *cfg;
  struct pci_dev * dev;
  int ret;

  if (copy_from_user(&iodev, (void __user *)arg, sizeof(iodev)))
    return -EFAULT;

  dev = pci_get_domain_bus_and_slot(iodev.domain, iodev.bus, iodev.devfn);
  if (!dev)
    return -ENODEV;

  cfg = vendor_reset_cfg_find(dev->vendor, dev->device);
  if (!cfg)
  {
    ret = -EOPNOTSUPP;
    goto err;
  }

  ret = vendor_reset_dev(cfg, dev);

err:
  pci_dev_put(dev);
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

static const struct file_operations vendor_reset_ioctl_fops =
{
  .owner          = THIS_MODULE,
  .unlocked_ioctl = vendor_reset_ioctl,
#ifdef CONFIG_COMPAT
  .compat_ioctl   = vendor_reset_ioctl,
#endif
};

static struct miscdevice vendor_reset_ioctl_misc =
{
  .minor = MISC_DYNAMIC_MINOR,
  .name  = VENDOR_RESET_IOCTL_DEVNAME,
  .fops  = &vendor_reset_ioctl_fops
};

int vendor_reset_ioctl_init(void)
{
  return misc_register(&vendor_reset_ioctl_misc);
}

void vendor_reset_ioctl_exit(void)
{
  misc_deregister(&vendor_reset_ioctl_misc);
}
