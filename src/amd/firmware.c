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

#include <linux/kernel.h>
#include "atom.h"
#include "amdgpu_atomfirmware.h"
#include "compat.h"
#include "common_defs.h"
#include "common.h"
#include "firmware.h"

#define to_adev(info) ((struct amd_fake_dev *)container_of(info, struct amd_fake_dev, card_info))
#define RREG32_IO(reg) amdgpu_io_rreg(adev, (reg))
#define WREG32_IO(reg, v) amdgpu_io_wreg(adev, (reg), (v))

/**
 * amdgpu_io_rreg - read an IO register
 *
 * @adev: amdgpu_device pointer
 * @reg: dword aligned register offset
 *
 * Returns the 32 bit value from the offset specified.
 */
u32 amdgpu_io_rreg(struct amd_fake_dev *adev, u32 reg)
{
  if ((reg * 4) < adev_to_amd_private(adev)->rio_mem_size)
    return ioread32(adev_to_amd_private(adev)->rio_mem + (reg * 4));
  else
  {
    iowrite32((reg * 4), adev_to_amd_private(adev)->rio_mem + (mmMM_INDEX * 4));
    return ioread32(adev_to_amd_private(adev)->rio_mem + (mmMM_DATA * 4));
  }
}

/**
 * amdgpu_io_wreg - write to an IO register
 *
 * @adev: amdgpu_device pointer
 * @reg: dword aligned register offset
 * @v: 32 bit value to write to the register
 *
 * Writes the value specified to the offset specified.
 */
void amdgpu_io_wreg(struct amd_fake_dev *adev, u32 reg, u32 v)
{
  if ((reg * 4) < adev_to_amd_private(adev)->rio_mem_size)
    iowrite32(v, adev_to_amd_private(adev)->rio_mem + (reg * 4));
  else
  {
    iowrite32((reg * 4), adev_to_amd_private(adev)->rio_mem + (mmMM_INDEX * 4));
    iowrite32(v, adev_to_amd_private(adev)->rio_mem + (mmMM_DATA * 4));
  }
}

static uint32_t null_read(struct card_info *info, uint32_t reg)
{
  return 0;
}

static void null_write(struct card_info *info, uint32_t reg, uint32_t val)
{
}

static void reg_write(struct card_info *info, uint32_t reg, uint32_t val)
{
  struct amd_fake_dev *adev = to_adev(info);

  WREG32(reg, val);
}

static uint32_t reg_read(struct card_info *info, uint32_t reg)
{
  struct amd_fake_dev *adev = to_adev(info);
  uint32_t r;

  r = RREG32(reg);
  return r;
}

static uint32_t ioreg_read(struct card_info *info, uint32_t reg)
{
  struct amd_fake_dev *adev = to_adev(info);
  uint32_t r;

  r = RREG32_IO(reg);
  return r;
}

static void ioreg_write(struct card_info *info, uint32_t reg, uint32_t val)
{
  struct amd_fake_dev *adev = to_adev(info);

  WREG32_IO(reg, val);
}

int atom_bios_init(struct amd_fake_dev *adev)
{
  struct card_info *info = &adev->card_info;

  info->mc_read = info->pll_read = null_read;
  info->mc_write = info->pll_write = null_write;

  info->reg_read = reg_read;
  info->reg_write = reg_write;
  if (adev_to_amd_private(adev)->rio_mem)
  {
    info->ioreg_read = ioreg_read;
    info->ioreg_write = ioreg_write;
  }
  else
  {
    pr_warn("vendor-reset: using MMIO to access I/O\n");
    info->ioreg_read = reg_read;
    info->ioreg_write = reg_write;
  }

  adev->atom_context = amdgpu_atom_parse(info, adev->bios);
  if (!adev->atom_context)
  {
    atom_bios_fini(adev);
    return -ENOMEM;
  }

  amdgpu_atomfirmware_scratch_regs_init(adev);

  return 0;
}

void atom_bios_fini(struct amd_fake_dev *adev)
{
  if (adev->atom_context)
  {
    kfree(adev->atom_context->scratch);
    kfree(adev->atom_context->iio);
  }
  kfree(adev->atom_context);
  adev->atom_context = NULL;
}
