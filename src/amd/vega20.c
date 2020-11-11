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

#include <linux/delay.h>

#include "nbio_7_4_offset.h"
#include "nbio_7_4_sh_mask.h"
#include "thm_11_0_2_offset.h"
#include "thm_11_0_2_sh_mask.h"
#include "mp_9_0_offset.h"
#include "mp_9_0_sh_mask.h"
#include "hdp_4_0_offset.h"
#include "hdp_4_0_sh_mask.h"
#include "common.h"

#include "soc15.h"
#include "soc15_common.h"
#include "common_baco.h"
#include "vega20_ppsmc.h"
#include "psp_gfx_if.h"

static const struct soc15_baco_cmd_entry clean_baco_tbl[] =
{
  {CMD_WRITE, SOC15_REG_ENTRY(NBIF, 0, mmBIOS_SCRATCH_6), 0, 0, 0, 0},
  {CMD_WRITE, SOC15_REG_ENTRY(NBIF, 0, mmBIOS_SCRATCH_7), 0, 0, 0, 0},
};

extern int vega20_reg_base_init(struct amd_fake_dev *adev);

static int vega20_baco_get_state(struct amd_fake_dev *adev, enum BACO_STATE *state)
{
  uint32_t reg = RREG32_SOC15(NBIF, 0, mmBACO_CNTL);

  if (reg & BACO_CNTL__BACO_MODE_MASK)
    /* gfx has already entered BACO state */
    *state = BACO_STATE_IN;
  else
    *state = BACO_STATE_OUT;
  return 0;
}

static int vega20_baco_set_state(struct amd_fake_dev *adev, enum BACO_STATE state)
{
  enum BACO_STATE cur_state;
  uint32_t data;

  vega20_baco_get_state(adev, &cur_state);

  if (cur_state == state)
    return 0;

  if (state == BACO_STATE_IN)
  {
    data = RREG32_SOC15(THM, 0, mmTHM_BACO_CNTL);
    data |= 0x80000000;
    WREG32_SOC15(THM, 0, mmTHM_BACO_CNTL, data);

    if (smum_send_msg_to_smc_with_parameter(adev, PPSMC_MSG_EnterBaco, 0, NULL))
      return -EINVAL;
  }
  else
  {
    if (smum_send_msg_to_smc(adev, PPSMC_MSG_ExitBaco, NULL))
      return -EINVAL;

    if (!soc15_baco_program_registers(adev, clean_baco_tbl,
          ARRAY_SIZE(clean_baco_tbl)))
      return -EINVAL;
  }

  return 0;
}

static int amd_vega20_mode1_reset(struct amd_fake_dev *adev)
{
  int ret;
  uint32_t offset;

  offset = SOC15_REG_OFFSET(MP0, 0, mmMP0_SMN_C2PMSG_64);
  ret = psp_wait_for(adev, offset, 0x80000000, 0x8000FFFF, false);
  if (ret)
  {
    pci_warn(adev->pdev, "vega20: psp not working for mode1 reset\n");
    return ret;
  }

  WREG32(offset, GFX_CTRL_CMD_ID_MODE1_RST);
  msleep(500);
  offset = SOC15_REG_OFFSET(MP0, 0, mmMP0_SMN_C2PMSG_33);
  ret = psp_wait_for(adev, offset, 0x80000000, 0x80000000, false);

  if (ret)
  {
    pci_warn(adev->pdev, "vega20: psp mode1 reset failed\n");
    return ret;
  }

  pci_info(adev->pdev, "vega20: psp mode1 reset succeeded\n");
  return ret;
}

static int amd_vega20_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct amd_fake_dev *adev;
  int ret = 0, timeout;
  u32 sol;
  enum BACO_STATE baco_state;

  adev = &priv->adev;
  ret = amd_fake_dev_init(adev, dev);
  if (ret)
    return ret;

  ret = vega20_reg_base_init(&priv->adev);
  if (ret)
    goto free_adev;

  /* it's important we wait for the SOC to be ready */
  for (timeout = 100000; timeout; --timeout)
  {
    sol = RREG32_SOC15(MP0, 0, mmMP0_SMN_C2PMSG_81);
    if (sol != 0xFFFFFFFF && sol != 0)
      break;
    udelay(1);
  }

  vega20_baco_get_state(adev, &baco_state);
  if (sol == ~1L && baco_state != BACO_STATE_IN)
  {
    pci_warn(dev->pdev, "vega20: Timed out waiting for SOL to be valid\n");
    ret = -EINVAL;
    goto free_adev;
  }

  /* if there's no sign of life we usually can't reset */
  if (!sol)
  {
    pci_info(dev->pdev, "vega20: no SOL, not attempting BACO reset\n");
    goto free_adev;
  }

  /* first try a mode1 psp reset */
  amdgpu_atombios_scratch_regs_engine_hung(adev, true);
  ret = amd_vega20_mode1_reset(adev);
  if (!ret)
  {
    amdgpu_atombios_scratch_regs_engine_hung(adev, false);
    goto free_adev;
  }

  pci_info(dev->pdev, "vega20: falling back to BACO reset\n");
  ret = vega20_baco_set_state(adev, BACO_STATE_IN);
  if (ret)
  {
    pci_warn(dev->pdev, "vega20: enter BACO failed\n");
    goto free_adev;
  }

  ret = vega20_baco_set_state(adev, BACO_STATE_OUT);
  if (ret)
  {
    pci_warn(dev->pdev, "vega20: exit BACO failed\n");
    goto free_adev;
  }

  pci_info(dev->pdev, "vega20: BACO reset successful\n");

free_adev:
  amd_fake_dev_fini(adev);

  return ret;
}

const struct vendor_reset_ops amd_vega20_ops =
{
  .pre_reset = amd_common_pre_reset,
  .reset = amd_vega20_reset,
  .post_reset = amd_common_post_reset,
};
