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

#include "vendor-reset-dev.h"

#include "amd.h"
#include "common_defs.h"
#include "common.h"
#include "firmware.h"
#include "amdgpu_discovery.h"
#include "smu_v11_0.h"
#include "mp/mp_11_0_offset.h"
#include "mp/mp_11_0_sh_mask.h"
#include "nbio_2_3_offset.h"
#include "nv.h"
#include "psp_gfx_if.h"
#include "smu_v11_0_ppsmc.h"

extern bool amdgpu_get_bios(struct amd_fake_dev *adev);

static int amd_navi10_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct amd_fake_dev *adev;
  int ret = 0, timeout;
  u32 sol, smu_resp, mp1_intr, psp_bl_ready, tmp, offset;

  adev = &priv->adev;
  ret = amd_fake_dev_init(adev, dev);
  if (ret)
    return ret;

  ret = amdgpu_discovery_reg_base_init(adev);
  if (ret < 0)
  {
    vr_info(dev, "amdgpu_discovery_reg_base_init failed, using legacy method\n");
    switch (dev->info)
    {
    case AMD_NAVI10:
      navi10_reg_base_init(adev);
      break;
    case AMD_NAVI12:
      navi12_reg_base_init(adev);
      break;
    case AMD_NAVI14:
      navi14_reg_base_init(adev);
      break;
    default:
      vr_err(dev, "Unknown Navi type device: [%04x:%04x]\n", dev->pdev->vendor, dev->pdev->device);
      return -ENOTSUPP;
    }
  }

  if (!amdgpu_get_bios(adev))
  {
    vr_err(dev, "amdgpu_get_bios failed: %d\n", ret);
    ret = -ENOTSUPP;
    goto free_adev;
  }

  ret = atom_bios_init(adev);
  if (ret)
  {
    vr_err(dev, "atom_bios_init failed: %d\n", ret);
    goto free_adev;
  }

  /* it's important we wait for the SOC to be ready */
  for (timeout = 100000; timeout; --timeout)
  {
    sol = RREG32_SOC15(MP0, 0, mmMP0_SMN_C2PMSG_81);
    if (sol != 0xFFFFFFFF && sol != 0)
      break;
    udelay(1);
  }

  if (sol == ~1L)
  {
    vr_warn(dev, "Timed out waiting for SOL to be valid\n");
    /* continuing anyway because sometimes it can still be reset from here */
  }

  vr_info(dev, "bus reset disabled? %s\n", (dev->pdev->dev_flags & PCI_DEV_FLAGS_NO_BUS_RESET) ? "yes" : "no");

  /* collect some info for logging for now */
  smu_resp = RREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_90);
  mp1_intr = (RREG32_PCIE(MP1_Public |
                          (smnMP1_FIRMWARE_FLAGS & 0xffffffff)) &
              MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED_MASK) >>
             MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED__SHIFT;
  psp_bl_ready = !!(RREG32_SOC15(MP0, 0, mmMP0_SMN_C2PMSG_35) & 0x80000000L);
  vr_info(dev, "SMU response reg: %x, sol reg: %x, mp1 intr enabled? %s, bl ready? %s\n",
          smu_resp, sol, mp1_intr ? "yes" : "no",
          psp_bl_ready ? "yes" : "no");

  /* okay, if we're in this state, we're probably reset */
  if (sol == 0x0 && !mp1_intr && psp_bl_ready)
    goto free_adev;

  /* this tells the drivers nvram is lost and everything needs to be reset */
  vr_info(dev, "Clearing scratch regs 6 and 7\n");
  WREG32(adev->bios_scratch_reg_offset + 6, 0);
  WREG32(adev->bios_scratch_reg_offset + 7, 0);

  /* it only makes sense to reset mp1 if it's running
   * XXX: is this even necessary? in early testing, I ran into
   * situations where MP1 was alive but not responsive, but in
   * later testing I have not been able to replicate this scenario.
   */
  if (smu_resp != 0x01 && mp1_intr)
  {
    vr_info(dev, "MP1 reset\n");
    WREG32_PCIE(MP1_Public | (smnMP1_PUB_CTRL & 0xffffffff),
                1 & MP1_SMN_PUB_CTRL__RESET_MASK);
    WREG32_PCIE(MP1_Public | (smnMP1_PUB_CTRL & 0xffffffff),
                1 & ~MP1_SMN_PUB_CTRL__RESET_MASK);

    vr_info(dev, "wait for MP1\n");
    for (timeout = 100000; timeout; --timeout)
    {
      tmp = RREG32_PCIE(MP1_Public |
                        (smnMP1_FIRMWARE_FLAGS & 0xffffffff));
      if ((tmp &
           MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED_MASK) >>
          MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED__SHIFT)
        break;
      udelay(1);
    }

    if (!timeout &&
        !((tmp & MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED_MASK) >>
          MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED__SHIFT))
    {
      vr_warn(dev, "timed out waiting for MP1 reset\n");
    }

    smu_wait(adev);
    smu_resp = RREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_90);
    vr_info(dev, "SMU resp reg: %x\n", tmp);
  }

  /*
   * again, this only makes sense if we have an SMU to talk to
   * some of these may fail, that's okay. we're just turning off as many
   * things as possible
   */
  if (mp1_intr)
  {
    smum_send_msg_to_smc(adev, PPSMC_MSG_DisallowGfxOff, NULL);
    smum_send_msg_to_smc(adev, PPSMC_MSG_PrepareMp1ForReset, NULL);
  }

  vr_info(dev, "begin psp mode 1 reset\n");
  amdgpu_atombios_scratch_regs_engine_hung(adev, true);

  pci_save_state(dev->pdev);

  /* check validity of PSP before reset */
  offset = SOC15_REG_OFFSET(MP0, 0, mmMP0_SMN_C2PMSG_64);
  tmp = psp_wait_for(adev, offset, 0x80000000, 0x8000FFFF, false);
  if (tmp)
    vr_warn(dev, "timed out waiting for PSP to reach valid state, but continuing anyway\n");

  /* reset command */
  WREG32_SOC15(MP0, 0, mmMP0_SMN_C2PMSG_64, GFX_CTRL_CMD_ID_MODE1_RST);
  msleep(500);

  /* wait for ACK */
  offset = SOC15_REG_OFFSET(MP0, 0, mmMP0_SMN_C2PMSG_33);
  tmp = psp_wait_for(adev, offset, 0x80000000, 0x80000000, false);
  if (tmp)
  {
    vr_warn(dev, "PSP did not acknowledger reset\n");
    ret = -EINVAL;
    goto out;
  }

  vr_info(dev, "mode1 reset succeeded\n");

  pci_restore_state(dev->pdev);

  for (timeout = 100000; timeout; --timeout)
  {
    tmp = RREG32_SOC15(NBIO, 0, mmRCC_DEV0_EPF0_RCC_CONFIG_MEMSIZE);

    if (tmp != 0xffffffff)
      break;
    udelay(1);
  }

  /*
   * this takes a long time :(
   */
  for (timeout = 100; timeout; --timeout)
  {
    /* see if PSP bootloader comes back */
    if (RREG32_SOC15(MP0, 0, mmMP0_SMN_C2PMSG_35) & 0x80000000L)
      break;
    msleep(100);
  }

  if (!timeout && !(RREG32_SOC15(MP0, 0, mmMP0_SMN_C2PMSG_35) & 0x80000000L))
  {
    vr_warn(dev, "timed out waiting for PSP bootloader to respond after reset\n");
    ret = -ETIME;
  }
  else
    vr_info(dev, "PSP mode1 reset successful\n");

out:
  pci_restore_state(dev->pdev);
  amdgpu_atombios_scratch_regs_engine_hung(adev, false);

free_adev:
  amd_fake_dev_fini(adev);

  return ret;
}

const struct vendor_reset_ops amd_navi10_ops =
{
  .version = {1, 1},
  .probe = amd_common_probe,
  .pre_reset = amd_common_pre_reset,
  .reset = amd_navi10_reset,
  .post_reset = amd_common_post_reset,
};
