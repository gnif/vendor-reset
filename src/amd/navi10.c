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

#include "common.h"
#include "firmware.h"
#include "amdgpu_discovery.h"
#include "nv.h"

extern bool amdgpu_get_bios(struct amd_fake_dev *adev);

static int amd_navi10_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct amd_fake_dev *adev;
  int ret;

  adev = &priv->adev;
  ret = amd_fake_dev_init(adev, dev);
  if (ret)
    return ret;

  ret = amdgpu_discovery_reg_base_init(adev);
  if (ret < 0)
  {
    pci_info(dev->pdev,
             "amdgpu_discovery_reg_base_init failed, using legacy method\n");
    navi10_reg_base_init(adev);
  }

  if (!amdgpu_get_bios(adev))
  {
    pci_err(dev->pdev, "amdgpu_get_bios failed: %d\n", ret);
    ret = -ENOTSUPP;
    goto adev_free;
  }

  ret = atom_bios_init(adev);
  if (ret)
  {
    pci_err(dev->pdev, "atom_bios_init failed: %d\n", ret);
    goto adev_free;
  }

adev_free:
  amd_fake_dev_fini(adev);

  return ret;
}

const struct vendor_reset_ops amd_navi10_ops =
{
  .pre_reset = amd_common_pre_reset,
  .reset = amd_navi10_reset,
  .post_reset = amd_common_post_reset,
};
