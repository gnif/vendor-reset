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

#ifndef __VENDOR_RESET_COMPAT_H__
#define __VENDOR_RESET_COMPAT_H__

#include <linux/types.h>

/* from amdgpu.h */
enum amd_hw_ip_block_type
{
  GC_HWIP = 1,
  HDP_HWIP,
  SDMA0_HWIP,
  SDMA1_HWIP,
  SDMA2_HWIP,
  SDMA3_HWIP,
  SDMA4_HWIP,
  SDMA5_HWIP,
  SDMA6_HWIP,
  SDMA7_HWIP,
  MMHUB_HWIP,
  ATHUB_HWIP,
  NBIO_HWIP,
  MP0_HWIP,
  MP1_HWIP,
  UVD_HWIP,
  VCN_HWIP = UVD_HWIP,
  JPEG_HWIP = VCN_HWIP,
  VCE_HWIP,
  DF_HWIP,
  DCE_HWIP,
  OSSSYS_HWIP,
  SMUIO_HWIP,
  PWR_HWIP,
  NBIF_HWIP,
  THM_HWIP,
  CLK_HWIP,
  UMC_HWIP,
  RSMU_HWIP,
  MAX_HWIP
};

#define HWIP_MAX_INSTANCE 8
/* end from amdgpu.h */

#include "amdgpu_gfx.h"
#include "amdgpu_ttm.h"
#include "atom.h"

struct amd_fake_dev
{
  uint32_t *reg_offset[MAX_HWIP][HWIP_MAX_INSTANCE];
  struct amdgpu_gfx gfx;
  struct amdgpu_mman mman;

  uint32_t bios_scratch_reg_offset;

  uint32_t bios_size;
  uint8_t *bios;

  struct card_info card_info;
  struct atom_context *atom_context;

  struct vendor_reset_dev *vdev;
  struct pci_dev *pdev;
  struct device *dev;
};

struct vendor_reset_dev;
int amd_fake_dev_init(struct amd_fake_dev *adev, struct vendor_reset_dev *vdev);
void amd_fake_dev_fini(struct amd_fake_dev *adev);

#endif
