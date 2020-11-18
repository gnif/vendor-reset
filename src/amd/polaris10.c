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

#include "atom-types.h"
#include "atombios.h"
#include "bif/bif_4_1_d.h"
#include "common.h"
#include "compat.h"
#include "polaris_baco.h"
#include "smu/smu_7_1_1_d.h"
#include "smu/smu_7_1_1_sh_mask.h"
#include "smu7_baco.h"
#include "vendor-reset-dev.h"

#define AMDGPU_ASIC_RESET_DATA 0x39d5e86b /* from amdgpu.h */
#define RREG32_SMC(reg) vi_smc_rreg(adev, reg)
#define WREG32_SMC(reg, v) vi_smc_wreg(adev, reg, v)

/* from vi.c */
static u32 vi_smc_rreg(struct amd_fake_dev *adev, u32 reg)
{
  u32 r;

  WREG32_NO_KIQ(mmSMC_IND_INDEX_11, (reg));
  r = RREG32_NO_KIQ(mmSMC_IND_DATA_11);
  return r;
}

/* not needed yet
static void vi_smc_wreg(struct amd_fake_dev *adev, u32 reg, u32 v)
{
  WREG32_NO_KIQ(mmSMC_IND_INDEX_11, (reg));
  WREG32_NO_KIQ(mmSMC_IND_DATA_11, (v));
}
*/

static int vi_gpu_pci_config_reset(struct amd_fake_dev *adev)
{
  u32 i;

  vr_info(adev->vdev, "GPU pci config reset\n");

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

static bool vi_need_reset_on_init(struct amd_fake_dev *adev)
{
  u32 clock_cntl, pc;

  /* check if the SMC is already running */
  clock_cntl = RREG32_SMC(ixSMC_SYSCON_CLOCK_CNTL_0);
  pc = RREG32_SMC(ixSMC_PC_C);
  vr_info(adev->vdev, "CLOCK_CNTL: 0x%x, PC: 0x%x\n", REG_GET_FIELD(clock_cntl, SMC_SYSCON_CLOCK_CNTL_0, ck_disable), pc);
  if ((0 == REG_GET_FIELD(clock_cntl, SMC_SYSCON_CLOCK_CNTL_0, ck_disable)) &&
      (0x20100 <= pc))
    return true;

  return false;
}
/* end from vi.c */

static int amd_polaris10_reset(struct vendor_reset_dev *vdev)
{
  int ret = 0;
  struct amd_vendor_private *priv = amd_private(vdev);
  struct amd_fake_dev *adev = &priv->adev;
  bool baco_capable;

  ret = amd_fake_dev_init(adev, vdev);
  if (ret)
    return ret;

  /* pre-firmware constant */
  adev->bios_scratch_reg_offset = mmBIOS_SCRATCH_0;

  if (!vi_need_reset_on_init(adev))
    goto free_adev;

  amdgpu_atombios_scratch_regs_engine_hung(adev, true);

  (void)smu7_baco_get_capability(adev, &baco_capable);
  if (baco_capable)
  {
    vr_info(vdev, "Performing BACO reset\n");
    ret = polaris_baco_set_state(adev, BACO_STATE_IN);
    if (ret)
    {
      vr_warn(vdev, "Failed to enter BACO: %d\n", ret);
      goto unhang;
    }
    ret = polaris_baco_set_state(adev, BACO_STATE_OUT);
    if (ret)
      vr_warn(vdev, "Failed to exit BACO: %d\n", ret);
  }
  else
  {
    vr_info(vdev, "Performing reset via PCI config\n");
    ret = vi_gpu_pci_config_reset(adev);
    if (ret)
      vr_warn(vdev, "Reset via PCI config failed: %d\n", ret);
  }

unhang:
  amdgpu_atombios_scratch_regs_engine_hung(adev, false);

free_adev:
  amd_fake_dev_fini(adev);
  return ret;
}

const struct vendor_reset_ops amd_polaris10_ops = {
    .version = {1, 1},
    .probe = amd_common_probe,
    .pre_reset = amd_common_pre_reset,
    .reset = amd_polaris10_reset,
    .post_reset = amd_common_post_reset,
};
