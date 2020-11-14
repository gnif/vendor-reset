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
#include "device-db.h"

struct vendor_reset_cfg * vendor_reset_cfg_find(unsigned int vendor,
  unsigned int device)
{
  struct vendor_reset_cfg * cfg;

  for(cfg = vendor_reset_devices; cfg->vendor; ++cfg)
  {
    if (cfg->vendor != vendor)
      continue;

    if (device == PCI_ANY_ID || device == cfg->device)
      break;
  }

  if (!cfg->vendor)
    return NULL;

  return cfg;
}

long vendor_reset_dev_locked(struct vendor_reset_cfg *cfg, struct pci_dev *dev)
{
  struct vendor_reset_dev vdev =
  {
    .cfg  = cfg,
    .pdev = dev,
    .info = cfg->info
  };
  int ret;

  vr_info(&vdev, "version %d.%d\n",
      cfg->ops->version.major,
      cfg->ops->version.minor);

  if (cfg->ops->pre_reset)
  {
    ret = cfg->ops->pre_reset(&vdev);
    if (ret)
      return ret;
  }

  /* expose return code to cleanup */
  ret = vdev.reset_ret = cfg->ops->reset(&vdev);
  if (ret)
    vr_warn(&vdev, "failed to reset device\n");

  if (cfg->ops->post_reset)
    ret = cfg->ops->post_reset(&vdev);

  return ret;
}

long vendor_reset_dev(struct vendor_reset_cfg *cfg, struct pci_dev *dev)
{
  int ret;

  if (!pci_cfg_access_trylock(dev))
  {
    pci_warn(dev, "could not acquire cfg lock\n");
    ret = -EAGAIN;
    goto err;
  }

  if (!device_trylock(&dev->dev))
  {
    pci_warn(dev, "could not acquire device lock\n");
    ret = -EAGAIN;
    goto unlock;
  }

  ret = vendor_reset_dev_locked(cfg, dev);
  device_unlock(&dev->dev);

unlock:
  pci_cfg_access_unlock(dev);

err:
  return ret;
}
