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

#ifndef __VENDOR_RESET_COMMON_H__
#define __VENDOR_RESET_COMMON_H__

#include <linux/kernel.h>
#include "vendor-reset-dev.h"

/* from amdgpu_discovery.c */
#ifndef mmMM_INDEX
#define mmRCC_CONFIG_MEMSIZE 0xde3
#define mmMM_INDEX 0x0
#define mmMM_INDEX_HI 0x6
#define mmMM_DATA 0x1
#define HW_ID_MAX 300
#endif
/* end from amdgpu_discovery.c */

#define RREG32(reg)                                                          \
  ({                                                                         \
    u32 __out;                                                               \
    if ((reg) < adev_to_amd_private(adev)->mmio_size)                        \
      __out = readl(adev_to_amd_private(adev)->mmio + (reg));                \
    else                                                                     \
    {                                                                        \
      unsigned long __flags;                                                 \
      spin_lock_irqsave(&adev_to_amd_private(adev)->reg_lock, __flags);      \
      writel((reg), adev_to_amd_private(adev)->mmio + mmMM_INDEX);           \
      __out = readl(adev_to_amd_private(adev)->mmio + mmMM_DATA);            \
      spin_unlock_irqrestore(&adev_to_amd_private(adev)->reg_lock, __flags); \
    }                                                                        \
    __out;                                                                   \
  })

#define WREG32(reg, v)                                                       \
  do                                                                         \
  {                                                                          \
    if ((reg) < adev_to_amd_private(adev)->mmio_size)                        \
      writel(v, adev_to_amd_private(adev)->mmio + (reg));                    \
    else                                                                     \
    {                                                                        \
      unsigned long __flags;                                                 \
      spin_lock_irqsave(&adev_to_amd_private(adev)->reg_lock, __flags);      \
      writel((reg), adev_to_amd_private(adev)->mmio + mmMM_INDEX);           \
      writel(v, adev_to_amd_private(adev)->mmio + mmMM_DATA);                \
      spin_unlock_irqrestore(&adev_to_amd_private(adev)->reg_lock, __flags); \
    }                                                                        \
  } while (0)

#define WREG32_PCIE(reg, v)                                                 \
  do                                                                        \
  {                                                                         \
    unsigned long __flags;                                                  \
    spin_lock_irqsave(&adev_to_amd_private(adev)->pcie_lock, __flags);      \
    WREG32(mmPCIE_INDEX2, reg);                                             \
    (void)RREG32(mmPCIE_INDEX2);                                            \
    WREG32(mmPCIE_DATA2, v);                                                \
    (void)RREG32(mmPCIE_DATA2);                                             \
    spin_unlock_irqrestore(&adev_to_amd_private(adev)->pcie_lock, __flags); \
  } while (0)

#define RREG32_PCIE(reg)                                                    \
  ({                                                                        \
    unsigned long __flags;                                                  \
    u32 __tmp_read;                                                         \
    spin_lock_irqsave(&adev_to_amd_private(adev)->pcie_lock, __flags);      \
    WREG32(mmPCIE_INDEX2, reg);                                             \
    (void)RREG32(mmPCIE_INDEX2);                                            \
    __tmp_read = RREG32(mmPCIE_DATA2);                                      \
    spin_unlock_irqrestore(&adev_to_amd_private(adev)->pcie_lock, __flags); \
    __tmp_read;                                                             \
  })

/* KIQ is only used for SRIOV accesses, we are not targetting these devices so
 * we can safely just wrap the defines */
#define WREG32_NO_KIQ WREG32
#define RREG32_NO_KIQ RREG32

/* from smu_cm.c */
/*
 * Although these are defined in each ASIC's specific header file.
 * They share the same definitions and values. That makes common
 * APIs for SMC messages issuing for all ASICs possible.
 */
#define mmMP1_SMN_C2PMSG_66 0x0282
#define mmMP1_SMN_C2PMSG_66_BASE_IDX 0

#define mmMP1_SMN_C2PMSG_82 0x0292
#define mmMP1_SMN_C2PMSG_82_BASE_IDX 0

#define mmMP1_SMN_C2PMSG_90 0x029a
#define mmMP1_SMN_C2PMSG_90_BASE_IDX 0

#define MP1_C2PMSG_90__CONTENT_MASK 0xFFFFFFFFL
/* end from smu_cm.c */

#include "compat.h"

struct amd_vendor_private
{
  u16 cfg;

  struct vendor_reset_dev *vdev;
  struct pci_saved_state *saved_state;
  struct amd_fake_dev adev;

  resource_size_t mmio_base;
  resource_size_t mmio_size;
  uint32_t __iomem *mmio;

  spinlock_t pcie_lock;
  spinlock_t reg_lock;
  struct mutex smu_lock;
};

#define adev_to_amd_private(adev) ((struct amd_vendor_private *)container_of(adev, struct amd_vendor_private, adev))
#define amd_private(vdev) ((struct amd_vendor_private *)(vdev->vendor_private))

int amd_common_pre_reset(struct vendor_reset_dev *);
int amd_common_post_reset(struct vendor_reset_dev *);

int smum_send_msg_to_smc(struct amd_fake_dev *adev, uint16_t msg, uint32_t *resp);
int smum_send_msg_to_smc_with_parameter(struct amd_fake_dev *adev, uint16_t msg, uint32_t parameter, uint32_t *resp);

#endif
