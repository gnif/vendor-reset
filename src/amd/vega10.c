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
#include "soc15.h"
#include "soc15_hw_ip.h"
#include "vega10_ip_offset.h"
#include "soc15_common.h"
#include "vega10_inc.h"
#include "vega10_ppsmc.h"
#include "vendor-reset-dev.h"
#include "firmware.h"
#include "common.h"
#include "compat.h"
#include "common_baco.h"

/* MP Apertures, from smu9_smumgr.c */
#define MP0_Public 0x03800000
#define MP0_SRAM 0x03900000
#define MP1_Public 0x03b00000
#define MP1_SRAM 0x03c00004

#define smnMP1_FIRMWARE_FLAGS 0x3010028

static const char *log_prefix = "vega10";
#define nv_info(fmt, arg...) pci_info(dev->pdev, "%s: " fmt, log_prefix, ##arg)
#define nv_warn(fmt, arg...) pci_warn(dev->pdev, "%s: " fmt, log_prefix, ##arg)
#define nv_err(fmt, arg...) pci_err(dev->pdev, "%s: " fmt, log_prefix, ##arg)

extern int vega10_reg_base_init(struct amd_fake_dev *adev);
extern bool amdgpu_get_bios(struct amd_fake_dev *adev);

/* drivers/gpu/drm/amd/powerplay/hwmgr/vega10_baco.c */
static const struct soc15_baco_cmd_entry pre_baco_tbl[] = {
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBIF_DOORBELL_CNTL), BIF_DOORBELL_CNTL__DOORBELL_MONITOR_EN_MASK, BIF_DOORBELL_CNTL__DOORBELL_MONITOR_EN__SHIFT, 0, 1},
    {CMD_WRITE, SOC15_REG_ENTRY(NBIF, 0, mmBIF_FB_EN), 0, 0, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_DSTATE_BYPASS_MASK, BACO_CNTL__BACO_DSTATE_BYPASS__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_RST_INTR_MASK_MASK, BACO_CNTL__BACO_RST_INTR_MASK__SHIFT, 0, 1},
};

static const struct soc15_baco_cmd_entry enter_baco_tbl[] = {
    {CMD_WAITFOR, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__SOC_DOMAIN_IDLE_MASK, THM_BACO_CNTL__SOC_DOMAIN_IDLE__SHIFT, 0xffffffff, 0x80000000},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_EN_MASK, BACO_CNTL__BACO_EN__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_BIF_LCLK_SWITCH_MASK, BACO_CNTL__BACO_BIF_LCLK_SWITCH__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_DUMMY_EN_MASK, BACO_CNTL__BACO_DUMMY_EN__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_SOC_VDCI_RESET_MASK, THM_BACO_CNTL__BACO_SOC_VDCI_RESET__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_SMNCLK_MUX_MASK, THM_BACO_CNTL__BACO_SMNCLK_MUX__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_ISO_EN_MASK, THM_BACO_CNTL__BACO_ISO_EN__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_AEB_ISO_EN_MASK, THM_BACO_CNTL__BACO_AEB_ISO_EN__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_ANA_ISO_EN_MASK, THM_BACO_CNTL__BACO_ANA_ISO_EN__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_SOC_REFCLK_OFF_MASK, THM_BACO_CNTL__BACO_SOC_REFCLK_OFF__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_POWER_OFF_MASK, BACO_CNTL__BACO_POWER_OFF__SHIFT, 0, 1},
    {CMD_DELAY_MS, 0, 0, 0, 0, 0, 0, 5, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_RESET_EN_MASK, THM_BACO_CNTL__BACO_RESET_EN__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_PWROKRAW_CNTL_MASK, THM_BACO_CNTL__BACO_PWROKRAW_CNTL__SHIFT, 0, 0},
    {CMD_WAITFOR, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_MODE_MASK, BACO_CNTL__BACO_MODE__SHIFT, 0xffffffff, 0x100},
};

static const struct soc15_baco_cmd_entry exit_baco_tbl[] = {
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_POWER_OFF_MASK, BACO_CNTL__BACO_POWER_OFF__SHIFT, 0, 0},
    {CMD_DELAY_MS, 0, 0, 0, 0, 0, 0, 10, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_SOC_REFCLK_OFF_MASK, THM_BACO_CNTL__BACO_SOC_REFCLK_OFF__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_ANA_ISO_EN_MASK, THM_BACO_CNTL__BACO_ANA_ISO_EN__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_AEB_ISO_EN_MASK, THM_BACO_CNTL__BACO_AEB_ISO_EN__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_ISO_EN_MASK, THM_BACO_CNTL__BACO_ISO_EN__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_PWROKRAW_CNTL_MASK, THM_BACO_CNTL__BACO_PWROKRAW_CNTL__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_SMNCLK_MUX_MASK, THM_BACO_CNTL__BACO_SMNCLK_MUX__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_SOC_VDCI_RESET_MASK, THM_BACO_CNTL__BACO_SOC_VDCI_RESET__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_EXIT_MASK, THM_BACO_CNTL__BACO_EXIT__SHIFT, 0, 1},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_RESET_EN_MASK, THM_BACO_CNTL__BACO_RESET_EN__SHIFT, 0, 0},
    {CMD_WAITFOR, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_EXIT_MASK, 0, 0xffffffff, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(THM, 0, mmTHM_BACO_CNTL), THM_BACO_CNTL__BACO_SB_AXI_FENCE_MASK, THM_BACO_CNTL__BACO_SB_AXI_FENCE__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_DUMMY_EN_MASK, BACO_CNTL__BACO_DUMMY_EN__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_BIF_LCLK_SWITCH_MASK, BACO_CNTL__BACO_BIF_LCLK_SWITCH__SHIFT, 0, 0},
    {CMD_READMODIFYWRITE, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_EN_MASK, BACO_CNTL__BACO_EN__SHIFT, 0, 0},
    {CMD_WAITFOR, SOC15_REG_ENTRY(NBIF, 0, mmBACO_CNTL), BACO_CNTL__BACO_MODE_MASK, 0, 0xffffffff, 0},
};

static const struct soc15_baco_cmd_entry clean_baco_tbl[] = {
    {CMD_WRITE, SOC15_REG_ENTRY(NBIF, 0, mmBIOS_SCRATCH_6), 0, 0, 0, 0},
    {CMD_WRITE, SOC15_REG_ENTRY(NBIF, 0, mmBIOS_SCRATCH_7), 0, 0, 0, 0},
};

int vega10_baco_set_state(struct amd_fake_dev *adev, enum BACO_STATE state)
{

  if (state == BACO_STATE_IN)
  {
    if (soc15_baco_program_registers(adev, pre_baco_tbl,
                                     ARRAY_SIZE(pre_baco_tbl)))
    {
      if (smum_send_msg_to_smc(adev, PPSMC_MSG_EnterBaco, NULL))
        return -EINVAL;

      if (soc15_baco_program_registers(adev, enter_baco_tbl,
                                       ARRAY_SIZE(enter_baco_tbl)))
        return 0;
    }
  }
  else if (state == BACO_STATE_OUT)
  {
    /* HW requires at least 20ms between regulator off and on */
    msleep(20);
    /* Execute Hardware BACO exit sequence */
    if (soc15_baco_program_registers(adev, exit_baco_tbl,
                                     ARRAY_SIZE(exit_baco_tbl)))
    {
      if (soc15_baco_program_registers(adev, clean_baco_tbl,
                                       ARRAY_SIZE(clean_baco_tbl)))
        return 0;
    }
  }

  return -EINVAL;
}

static int amd_vega10_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct amd_fake_dev *adev;
  int ret = 0, timeout;
  u32 sol, smu_resp, mp1_intr, psp_bl_ready, features_mask;
  enum BACO_STATE baco_state;

  adev = &priv->adev;
  ret = amd_fake_dev_init(adev, dev);
  if (ret)
    return ret;

  ret = vega10_reg_base_init(&priv->adev);
  if (ret)
    goto free_adev;

  if (!amdgpu_get_bios(adev))
  {
    nv_err("amdgpu_get_bios failed: %d\n", ret);
    ret = -ENOTSUPP;
    goto free_adev;
  }

  ret = atom_bios_init(adev);
  if (ret)
  {
    nv_err("atom_bios_init failed: %d\n", ret);
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

  pci_info(dev->pdev, "Vega10: bus reset disabled? %s\n", (dev->pdev->dev_flags & PCI_DEV_FLAGS_NO_BUS_RESET) ? "yes" : "no");

  /* collect some info for logging for now */
  smu_resp = RREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_90);
  mp1_intr = (RREG32_PCIE(MP1_Public |
                          (smnMP1_FIRMWARE_FLAGS & 0xffffffff)) &
              MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED_MASK) >>
             MP1_FIRMWARE_FLAGS__INTERRUPTS_ENABLED__SHIFT;
  psp_bl_ready = !!(RREG32(mmMP0_SMN_C2PMSG_35) & 0x80000000L);
  smu9_baco_get_state(adev, &baco_state);
  pci_info(
      dev->pdev,
      "Vega10: SMU response reg: %x, sol reg: %x, mp1 intr enabled? %s, bl ready? %s, baco? %s\n",
      smu_resp, sol, mp1_intr ? "yes" : "no",
      psp_bl_ready ? "yes" : "no",
      baco_state == BACO_STATE_IN ? "on" : "off");

  if (sol == ~1L && baco_state != BACO_STATE_IN)
  {
    pci_warn(dev->pdev, "Vega10: Timed out waiting for SOL to be valid\n");
    ret = -EINVAL;
    goto free_adev;
  }

  /* if there's no sign of life we usually can't reset */
  if (!sol)
    goto free_adev;

  /* disable smu features */
  ret = smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_GetEnabledSmuFeatures, 0, &features_mask);
  if (ret)
  {
    nv_warn("Could not get enabled SMU features, trying BACO reset anyway [ret %d]\n", ret);
    goto baco_reset;
  }

  nv_info("Enabled features: %x\n", features_mask);

  /*
   * Based on the following observed sequence:
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00800000	ret=          	features=GNLD_FW_CTF
   * ...
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00010000	ret=          	features=GNLD_THERMAL
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00004000	ret=          	features=GNLD_PPT
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00008000	ret=          	features=GNLD_TDC
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x08000000	ret=          	features=GNLD_DIDT
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x01000000	ret=          	features=GNLD_LED_DISPLAY
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x0000030f	ret=          	features=GNLD_DPM_PREFETCHER|GNLD_DPM_GFXCLK|GNLD_DPM_UCLK|GNLD_DPM_SOCCLK|GNLD_DPM_LINK|GNLD_DPM_DCEFCLK
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00000400	ret=          	features=GNLD_AVFS
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00000800	ret=          	features=GNLD_DS_GFXCLK
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00001000	ret=          	features=GNLD_DS_SOCCLK
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00002000	ret=          	features=GNLD_DS_LCLK
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00080000	ret=          	features=GNLD_DS_DCEFCLK
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x00000040	ret=          	features=GNLD_ULV
   * cmd=PPSMC_MSG_DisableSmuFeatures            	param=0x10000000	ret=          	features=GNLD_ACG
   * cmd=PPSMC_MSG_GfxDeviceDriverReset          	param=0x00000002	ret=
   * 
   * However, this sequence bricks the card after shutting down Windows,
   * so instead we'll mask the difference between the macOS shutdown feature
   * list (0x1bb9ff1f) and the Windows shutdown feature list (0x1320070f),
   * using the above sequence as ordering for the bits remaining.
   */

  nv_info("Disabling features\n");
  if (features_mask & 0x00800000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00800000, NULL);
  if (features_mask & 0x00010000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00010000, NULL);
  if (features_mask & 0x00004000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00004000, NULL);
  if (features_mask & 0x00008000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00008000, NULL);
  if (features_mask & 0x08000000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x08000000, NULL);
  if (features_mask & 0x00000010)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00000010, NULL);
  if (features_mask & 0x00000800)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00000800, NULL);
  if (features_mask & 0x00001000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00001000, NULL);
  if (features_mask & 0x00002000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00002000, NULL);
  if (features_mask & 0x00080000)
    smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_DisableSmuFeatures, 0x00080000, NULL);

  /* driver reset */
  nv_info("Driver reset\n");
  ret = smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_GfxDeviceDriverReset, 0x2, NULL);
  if (ret)
    nv_warn("Could not reset w/ PPSMC_MSG_GfxDeviceDriverReset: %d\n", ret);

  /* no reference for this, observed timing appears to be ~500ms */
  msleep(500);

baco_reset:
  if (baco_state == BACO_STATE_OUT)
  {
    pci_info(dev->pdev, "Vega10: Entering BACO\n");
    ret = vega10_baco_set_state(adev, BACO_STATE_IN);
    if (ret)
      return ret;
  }

  pci_info(dev->pdev, "Vega10: Exiting BACO\n");
  ret = vega10_baco_set_state(adev, BACO_STATE_OUT);

free_adev:
  amd_fake_dev_fini(adev);

  return ret;
}

const struct vendor_reset_ops amd_vega10_ops = {
  .pre_reset = amd_common_pre_reset,
  .reset = amd_vega10_reset,
  .post_reset = amd_common_post_reset,
};
