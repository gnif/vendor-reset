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
#include "common.h"
#include "common_baco.h"

extern int vega10_reg_base_init(struct amd_fake_dev *adev);

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
  int ret, timeout;
  u32 sol;

  priv->adev = (struct amd_fake_dev){
      .dev = &dev->pdev->dev,
      .private = priv,
  };
  adev = &priv->adev;
  ret = vega10_reg_base_init(&priv->adev);
  if (ret)
    return ret;

  /* it's important we wait for the SOC to be ready */
  for (timeout = 100000; timeout; --timeout)
  {
    sol = RREG32(mmMP0_SMN_C2PMSG_81);
    if (sol != 0xFFFFFFFF)
      break;
    udelay(1);
  }

  if (sol == 0xFFFFFFFF)
  {
    pci_warn(dev->pdev, "Vega10: Timed out waiting for SOL to be valid\n");
    return -EINVAL;
  }

  pci_info(dev->pdev, "Vega10: Entering BACO\n");
  ret = vega10_baco_set_state(adev, BACO_STATE_IN);
  if (ret)
    return ret;

  pci_info(dev->pdev, "Vega10: Exiting BACO\n");
  ret = vega10_baco_set_state(adev, BACO_STATE_OUT);
  return ret;
}

const struct vendor_reset_ops amd_vega10_ops = {
	.pre_reset = amd_common_pre_reset,
	.reset = amd_vega10_reset,
	.post_reset = amd_common_post_reset,
};
