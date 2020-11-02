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

#include "amdgpu_discovery.h"

#include "common.h"
#include "compat.h"
#include "firmware.h"

int amd_fake_dev_init(struct amd_fake_dev *adev, struct vendor_reset_dev *vdev)
{
  adev->pdev = vdev->pdev;
  adev->dev = &vdev->pdev->dev;

  return 0;
}

void amd_fake_dev_fini(struct amd_fake_dev *adev)
{
  if (adev->mman.discovery_bin)
    amdgpu_discovery_fini(adev);

  if (adev->atom_context)
    atom_bios_fini(adev);

  if (adev->bios)
  {
    kfree(adev->bios);
    adev->bios = NULL;
  }
}