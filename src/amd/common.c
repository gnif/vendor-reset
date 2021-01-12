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

#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include "vendor-reset-dev.h"
#include "soc15_common.h"
#include "soc15.h"
#include "common_defs.h"
#include "common.h"
#include "compat.h"

int amd_common_probe(const struct vendor_reset_cfg *cfg, struct pci_dev *dev)
{
  /* disable bus reset for the card, seems to be an issue with all of them */
  dev->dev_flags |= PCI_DEV_FLAGS_NO_BUS_RESET;
  return 0;
}

int amd_common_pre_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv;
  struct pci_dev *pdev = dev->pdev;
  int ret, i;

  /* disable bus reset for the card, seems to be an issue with all of them */
  dev->pdev->dev_flags |= PCI_DEV_FLAGS_NO_BUS_RESET;

  /* do not try to reset the card under amdgpu, it will cause problems */
  if (pdev->driver && !strcmp(pdev->driver->name, "amdgpu"))
    return -ENOTTY;

  priv = kzalloc(sizeof *priv, GFP_KERNEL);
  if (!priv)
    return -ENOMEM;

  dev->vendor_private = priv;
  priv->vdev = dev;

  priv->mmio_base = pci_resource_start(pdev, 5);
  priv->mmio_size = pci_resource_len(pdev, 5);
  priv->mmio = ioremap(priv->mmio_base, priv->mmio_size);
  if (!priv->mmio)
  {
    pci_err(pdev, "Could not mmap device\n");
    ret = -ENOMEM;
    goto err_free;
  }

  for (i = 0; i < DEVICE_COUNT_RESOURCE; i++)
  {
    if (pci_resource_flags(pdev, i) & IORESOURCE_IO)
    {
      priv->rio_mem_size = pci_resource_len(pdev, i);
      priv->rio_mem = pci_iomap(pdev, i, priv->rio_mem_size);
      break;
    }
  }
  if (!priv->rio_mem)
    vr_warn(dev, "Could not map I/O\n");

  pci_set_power_state(pdev, PCI_D0);
  pci_clear_master(pdev);
  pci_save_state(pdev);
  priv->saved_state = pci_store_saved_state(pdev);
  pci_read_config_word(pdev, PCI_COMMAND, &priv->cfg);
  pci_write_config_word(pdev, PCI_COMMAND, priv->cfg | PCI_COMMAND_MEMORY | PCI_COMMAND_INTX_DISABLE);

  priv->audio_pdev = pci_get_domain_bus_and_slot(pci_domain_nr(pdev->bus),
                                                 pdev->bus->number, 1);
  if (priv->audio_pdev)
  {
    pci_set_power_state(priv->audio_pdev, PCI_D0);
    pci_clear_master(priv->audio_pdev);
    pci_save_state(priv->audio_pdev);
  }

  return 0;

err_free:
  kfree(priv);
  return ret;
}

int amd_common_post_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct pci_dev *pdev = dev->pdev;

  if (priv->mmio)
  {
    iounmap(priv->mmio);
    priv->mmio = NULL;
  }

  if (priv->rio_mem)
  {
    pci_iounmap(pdev, priv->rio_mem);
    priv->rio_mem = NULL;
  }

  if (priv->saved_state)
  {
    pci_load_and_free_saved_state(pdev, &priv->saved_state);
    pci_restore_state(pdev);
  }
  pci_write_config_word(pdev, PCI_COMMAND, priv->cfg);

  if (priv->audio_pdev)
  {
    pci_restore_state(priv->audio_pdev);
    pci_set_power_state(priv->audio_pdev, PCI_D3hot);
    pci_dev_put(priv->audio_pdev);
    priv->audio_pdev = NULL;
  }

  /* don't try to go to low power if reset failed */
  if (!dev->reset_ret)
    pci_set_power_state(pdev, PCI_D3hot);

  kfree(priv);
  dev->vendor_private = NULL;

  return 0;
}

int smu_wait(struct amd_fake_dev *adev)
{
  u32 ret;
  int timeout;

  for (timeout = 100000;
       timeout &&
       (RREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_90) & MP1_C2PMSG_90__CONTENT_MASK) == 0;
       --timeout)
    udelay(1);
  if ((ret = RREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_90)) != 0x1)
    pci_info(adev_to_amd_private(adev)->vdev->pdev, "SMU error 0x%x\n", ret);

  return ret;
}

int smum_send_msg_to_smc_with_parameter(struct amd_fake_dev *adev, uint16_t msg, uint32_t parameter, uint32_t *resp)
{
  int ret = 0;

  ret = smu_wait(adev);
  if (ret != 0x1)
  {
    ret = -ETIMEDOUT;
    goto out;
  }

  WREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_90, 0);
  WREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_82, parameter);

  WREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_66, msg);

  ret = smu_wait(adev);
  if (ret != 0x01)
  {
    pci_err(adev_to_amd_private(adev)->vdev->pdev, "Failed to send message 0x%x: return 0x%x\n", msg, ret);
    goto out;
  }

  if (resp)
    *resp = RREG32_SOC15(MP1, 0, mmMP1_SMN_C2PMSG_82);

  ret = ret != 0x01;

out:
  return ret;
}

int smum_send_msg_to_smc(struct amd_fake_dev *adev, uint16_t msg, uint32_t *resp)
{
  return smum_send_msg_to_smc_with_parameter(adev, msg, 0, resp);
}

/* from amdgpu_atombios.c */
void amdgpu_atombios_scratch_regs_engine_hung(struct amd_fake_dev *adev,
                                              bool hung)
{
  u32 tmp = RREG32(adev->bios_scratch_reg_offset + 3);

  if (hung)
    tmp |= ATOM_S3_ASIC_GUI_ENGINE_HUNG;
  else
    tmp &= ~ATOM_S3_ASIC_GUI_ENGINE_HUNG;

  WREG32(adev->bios_scratch_reg_offset + 3, tmp);
}

/* from amdgpu_psp.c */
int psp_wait_for(struct amd_fake_dev *adev, uint32_t reg_index,
                 uint32_t reg_val, uint32_t mask, bool check_changed)
{
  uint32_t val;
  int i;

  for (i = 0; i < 100000; i++)
  {
    val = RREG32(reg_index);
    if (check_changed)
    {
      if (val != reg_val)
        return 0;
    }
    else
    {
      if ((val & mask) == reg_val)
        return 0;
    }
    udelay(1);
  }

  return -ETIME;
}
