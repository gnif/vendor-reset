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

#include <linux/delay.h>

#include "bif/bif_4_1_d.h"
#include "atom-types.h"
#include "atombios.h"
#include "common.h"
#include "compat.h"

#define AMDGPU_ASIC_RESET_DATA 0x39d5e86b /* from amdgpu.h */

/* from vi.c */
static int vi_gpu_pci_config_reset(struct amd_fake_dev *adev)
{
  u32 i;

  dev_info(adev->dev, "GPU pci config reset\n");

  /* reset */
  pci_write_config_dword(adev->pdev, 0x7c, AMDGPU_ASIC_RESET_DATA);

  udelay(100);

  /* wait for asic to come out of reset */
  for (i = 0; i < 100000; i++)
  {
    if (RREG32(mmCONFIG_MEMSIZE) != 0xffffffff)
    {
      return 0;
    }
    udelay(1);
  }
  return -EINVAL;
}

static int amd_polaris10_reset(struct vendor_reset_dev *vdev)
{
  int ret = 0;
  struct amd_vendor_private *priv = amd_private(vdev);
  struct amd_fake_dev *adev = &priv->adev;

  ret = amd_fake_dev_init(adev, vdev);
  if (ret)
    return ret;

  /* pre-firmware constant */
  adev->bios_scratch_reg_offset = mmBIOS_SCRATCH_0;

  amdgpu_atombios_scratch_regs_engine_hung(adev, true);
  ret = vi_gpu_pci_config_reset(adev);
  amdgpu_atombios_scratch_regs_engine_hung(adev, false);

  amd_fake_dev_fini(adev);
  return ret;
}

const struct vendor_reset_ops amd_polaris10_ops = {
    .pre_reset = amd_common_pre_reset,
    .reset = amd_polaris10_reset,
    .post_reset = amd_common_post_reset,
};