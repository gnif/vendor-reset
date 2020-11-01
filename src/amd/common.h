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

#ifndef __VENDOR_RESET_COMMON_H__
#define __VENDOR_RESET_COMMON_H__

#include "vendor-reset-dev.h"

#define RREG32(reg)                     \
  ({                                    \
    u32 out;                            \
    if ((reg) < mmio_size)              \
      out = readl(mmio + (reg));        \
    else                                \
    {                                   \
      writel((reg), mmio + mmMM_INDEX); \
      out = readl(mmio + mmMM_DATA);    \
    }                                   \
    out;                                \
  })

#define WREG32(reg, v)                  \
  do                                    \
  {                                     \
    if ((reg) < mmio_size)              \
      writel(v, mmio + (reg));          \
    else                                \
    {                                   \
      writel((reg), mmio + mmMM_INDEX); \
      writel(v, mmio + mmMM_DATA);      \
    }                                   \
  } while (0)

#define WREG32_PCIE(reg, v)                      \
  do                                             \
  {                                              \
    unsigned long __flags;                       \
    spin_lock_irqsave(&pcie_lock, __flags);      \
    WREG32(mmPCIE_INDEX2, reg);                  \
    (void)RREG32(mmPCIE_INDEX2);                 \
    WREG32(mmPCIE_DATA2, v);                     \
    (void)RREG32(mmPCIE_DATA2);                  \
    spin_unlock_irqrestore(&pcie_lock, __flags); \
  } while (0)

#define RREG32_PCIE(reg)                         \
  ({                                             \
    unsigned long __flags;                       \
    u32 __tmp_read;                              \
    spin_lock_irqsave(&pcie_lock, __flags);      \
    WREG32(mmPCIE_INDEX2, reg);                  \
    (void)RREG32(mmPCIE_INDEX2);                 \
    __tmp_read = RREG32(mmPCIE_DATA2);           \
    spin_unlock_irqrestore(&pcie_lock, __flags); \
    __tmp_read;                                  \
  })

struct amd_vendor_private
{
  u16 cfg;

  struct pci_saved_state *saved_state;

  spinlock_t pcie_lock;
};

#define amd_private(vdev) ((struct amd_vendor_private *)(vdev->vendor_private))

int amd_common_pre_reset(struct vendor_reset_dev *);
int amd_common_post_reset(struct vendor_reset_dev *);

#endif