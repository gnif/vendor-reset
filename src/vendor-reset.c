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

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/pci.h>

#include "vendor-reset-dev.h"
#include "vendor-reset-ioctl.h"

#include "ftrace.h"
#include "hooks.h"

#define VENDOR_RESET_DEVNAME "vendor_reset"

#define vr_info(fmt, args...) pr_info("vendor-reset: " fmt, ##args)

static bool install_hook = true;
module_param(install_hook, bool, 0);

static long vendor_reset_ioctl_reset(struct file * filp, unsigned long arg)
{
  struct vendor_reset_ioctl dev;
  struct vendor_reset_cfg *cfg;
  struct pci_dev * pcidev;
  int ret;

  if (copy_from_user(&dev, (void __user *)arg, sizeof(dev)))
    return -EFAULT;

  pcidev = pci_get_domain_bus_and_slot(dev.domain, dev.bus, dev.devfn);
  if (!pcidev)
    return -ENODEV;

  cfg = vendor_reset_cfg_find(pcidev->vendor, pcidev->device);
  if (!cfg)
  {
    ret = -EOPNOTSUPP;
    goto err;
  }

  /* we probably always want to lock the device */
  if (!pci_cfg_access_trylock(pcidev))
  {
    pci_warn(pcidev, "Could not acquire cfg lock\n");
    ret = -EAGAIN;
    goto err;
  }

  if (!device_trylock(&pcidev->dev))
  {
    pci_warn(pcidev, "Could not acquire device lock\n");
    ret = -EAGAIN;
    goto unlock;
  }

  ret = vendor_reset_dev_locked(cfg, pcidev);
  device_unlock(&pcidev->dev);

unlock:
  pci_cfg_access_unlock(pcidev);
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
  int ret;

  ret = misc_register(&vendor_reset_misc);
  if (ret)
    return ret;

  if (install_hook)
  {
    ret = fh_install_hooks(fh_hooks);
    if (ret)
      goto err;

    vr_info("Hooks installed successfully\n");
  }

  return 0;

err:
  misc_deregister(&vendor_reset_misc);
  return ret;
}

static void __exit vendor_reset_exit(void)
{
  if (install_hook)
    fh_remove_hooks(fh_hooks);

  misc_deregister(&vendor_reset_misc);
}

module_init(vendor_reset_init);
module_exit(vendor_reset_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Geoffrey McRae <geoff@hostfission.com>");
MODULE_AUTHOR("Adam Madsen <adam@ajmadsen.com>");
