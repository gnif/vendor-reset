/*
Vendor Reset - Vendor Specific Reset
SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <linux/delay.h>

#include "vendor-reset-dev.h"
#include "amd.h"
#include "common_defs.h"
#include "common.h"
#include "amdgpu_discovery.h"
#include "soc15_hw_ip.h"

/* Linux v6.16: mp_14_0_2_offset.h and smu_v14_0_2_ppt.c. */
#define NAVI48_DEBUG_PARAM 0x0075
#define NAVI48_DEBUG_RESP  0x0076
#define NAVI48_DEBUG_MSG   0x008b
#define NAVI48_PSP_BL_READY 0x0063
#define NAVI48_CONFIG_MEMSIZE 0x0de3
#define NAVI48_MODE1_RESET 2

static int amd_navi48_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct amd_fake_dev *adev = &priv->adev;
  u32 smu_base, psp_base, version, value;
  int ret, timeout;

  if (priv->mmio_size < (NAVI48_CONFIG_MEMSIZE + 1) * 4)
  {
    vr_err(dev, "Navi48 MMIO aperture is too small for discovery\n");
    return -EINVAL;
  }

  ret = amd_fake_dev_init(adev, dev);
  if (ret)
    return ret;

  ret = amdgpu_discovery_get_reg_base(adev, MP1_HWID, 1, &smu_base, &version);
  if (ret)
    goto out;
  if (version != 0x0e0002 && version != 0x0e0003)
  {
    vr_err(dev, "Unsupported Navi48 SMU version: 0x%x\n", version);
    ret = -ENOTSUPP;
    goto out;
  }

  ret = amdgpu_discovery_get_reg_base(adev, MP0_HWID, 0, &psp_base, &version);
  if (ret)
    goto out;
  if (version != 0x0e0002 && version != 0x0e0003)
  {
    vr_err(dev, "Unsupported Navi48 PSP version: 0x%x\n", version);
    ret = -ENOTSUPP;
    goto out;
  }

  if (!smu_base || !psp_base ||
      smu_base > 0x3fffffff - NAVI48_DEBUG_MSG ||
      psp_base > 0x3fffffff - NAVI48_PSP_BL_READY ||
      ((u64)smu_base + NAVI48_DEBUG_MSG + 1) * 4 > priv->mmio_size ||
      ((u64)psp_base + NAVI48_PSP_BL_READY + 1) * 4 > priv->mmio_size)
  {
    vr_err(dev, "Navi48 reset registers outside MMIO aperture\n");
    ret = -EINVAL;
    goto out;
  }

  value = RREG32(smu_base + NAVI48_DEBUG_RESP);
  if (value == 0xffffffff)
  {
    vr_err(dev, "Navi48 SMU debug mailbox is inaccessible\n");
    ret = -EIO;
    goto out;
  }

  vr_warn(dev, "Experimental Navi48 SMU mode1 reset\n");

  ret = pci_save_state(dev->pdev);
  if (ret)
  {
    vr_err(dev, "Could not save Navi48 PCI state: %d\n", ret);
    goto out;
  }

  /* SMU14 triggers the debug command by clearing the response last. */
  WREG32(smu_base + NAVI48_DEBUG_PARAM, 0);
  WREG32(smu_base + NAVI48_DEBUG_MSG, NAVI48_MODE1_RESET);
  WREG32(smu_base + NAVI48_DEBUG_RESP, 0);
  msleep(1000);

  pci_restore_state(dev->pdev);

  /* Match PSP14's bootloader-ready bit; an all-ones PCI read is not ready. */
  for (timeout = 1000; timeout; --timeout)
  {
    value = RREG32(psp_base + NAVI48_PSP_BL_READY);
    if (value != 0xffffffff && (value & 0x80000000))
      break;
    usleep_range(1000, 1100);
  }
  if (!timeout)
  {
    vr_err(dev, "Timed out waiting for Navi48 PSP bootloader\n");
    ret = -ETIMEDOUT;
    goto out;
  }

  for (timeout = 1000; timeout; --timeout)
  {
    value = RREG32(NAVI48_CONFIG_MEMSIZE);
    if (value && value != 0xffffffff)
      break;
    usleep_range(1000, 1100);
  }
  if (!timeout)
  {
    vr_err(dev, "Timed out waiting for Navi48 memory controller\n");
    ret = -ETIMEDOUT;
    goto out;
  }

  vr_info(dev, "Navi48 SMU mode1 reset completed\n");
  ret = 0;

out:
  amd_fake_dev_fini(adev);
  return ret;
}

static int amd_navi48_post_reset(struct vendor_reset_dev *dev)
{
  int ret = amd_common_post_reset(dev);

  /* The dispatcher returns post_reset's result, so retain reset failures. */
  return dev->reset_ret ? dev->reset_ret : ret;
}

const struct vendor_reset_ops amd_navi48_ops =
{
  .version = {1, 0},
  .probe = amd_common_probe,
  .pre_reset = amd_common_pre_reset,
  .reset = amd_navi48_reset,
  .post_reset = amd_navi48_post_reset,
};
