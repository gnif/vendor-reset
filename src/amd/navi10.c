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

#include "vendor-reset-dev.h"

#include "common.h"
#include "amdgpu_discovery.h"
#include "nv.h"

static int amd_navi10_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct amd_fake_dev *adev;
  int ret;

  priv->adev = (struct amd_fake_dev){
      .dev = &dev->pdev->dev,
      .private = priv,
  };
  adev = &priv->adev;

  ret = amdgpu_discovery_reg_base_init(adev);
  if (ret < 0)
  {
    pci_info(dev->pdev,
        "amdgpu_discovery_reg_base_init failed, using legacy method");
    navi10_reg_base_init(adev);
  }

  if (adev->mman.discovery_bin)
    amdgpu_discovery_fini(adev);

  return 0;
}

const struct vendor_reset_ops amd_navi10_ops =
{
	.pre_reset = amd_common_pre_reset,
  .reset = amd_navi10_reset,
	.post_reset = amd_common_post_reset,
};
