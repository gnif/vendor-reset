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

/* these are to remove the dependency on DRM */
#include <linux/kgdb.h>
#define DRM_INFO(fmt, args...) pr_info("vendor-reset-drm: " fmt, ##args)
#define DRM_ERROR(fmt, args...) pr_err("vendor-reset-drm: " fmt, ##args)
#define DRM_DEBUG(fmt, args...) pr_debug("vendor-reset-drm: " fmt, ##args)
static inline bool drm_can_sleep(void)
{
  if (in_atomic() || in_dbg_master() || irqs_disabled())
    return false;
  return true;
}

#define RREG32(reg)                                                    \
  ({                                                                   \
    u32 __out;                                                         \
    if (((reg)*4) < adev_to_amd_private(adev)->mmio_size)              \
      __out = readl(adev_to_amd_private(adev)->mmio + (reg));          \
    else                                                               \
    {                                                                  \
      writel(((reg)*4), adev_to_amd_private(adev)->mmio + mmMM_INDEX); \
      __out = readl(adev_to_amd_private(adev)->mmio + mmMM_DATA);      \
    }                                                                  \
    __out;                                                             \
  })

#define WREG32(reg, v)                                                 \
  do                                                                   \
  {                                                                    \
    if (((reg)*4) < adev_to_amd_private(adev)->mmio_size)              \
      writel(v, adev_to_amd_private(adev)->mmio + (reg));              \
    else                                                               \
    {                                                                  \
      writel(((reg)*4), adev_to_amd_private(adev)->mmio + mmMM_INDEX); \
      writel(v, adev_to_amd_private(adev)->mmio + mmMM_DATA);          \
    }                                                                  \
  } while (0)

#define WREG32_PCIE(reg, v)      \
  do                             \
  {                              \
    WREG32(mmPCIE_INDEX2, reg);  \
    (void)RREG32(mmPCIE_INDEX2); \
    WREG32(mmPCIE_DATA2, v);     \
    (void)RREG32(mmPCIE_DATA2);  \
  } while (0)

#define RREG32_PCIE(reg)               \
  ({                                   \
    u32 __tmp_read;                    \
    WREG32(mmPCIE_INDEX2, reg);        \
    (void)RREG32(mmPCIE_INDEX2);       \
    __tmp_read = RREG32(mmPCIE_DATA2); \
    __tmp_read;                        \
  })

/* KIQ is only used for SRIOV accesses, we are not targetting these devices so
 * we can safely just wrap the defines */
#define WREG32_NO_KIQ WREG32
#define RREG32_NO_KIQ RREG32

/* from amdgpu.h */
#define REG_FIELD_SHIFT(reg, field) reg##__##field##__SHIFT
#define REG_FIELD_MASK(reg, field) reg##__##field##_MASK
#define REG_SET_FIELD(orig_val, reg, field, field_val) \
  (((orig_val) & ~REG_FIELD_MASK(reg, field)) |        \
   (REG_FIELD_MASK(reg, field) &                       \
    ((field_val) << REG_FIELD_SHIFT(reg, field))))
#define REG_GET_FIELD(value, reg, field) \
  (((value)&REG_FIELD_MASK(reg, field)) >> REG_FIELD_SHIFT(reg, field))

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

  struct pci_dev *audio_pdev;

  struct vendor_reset_dev *vdev;
  struct pci_saved_state *saved_state;
  struct amd_fake_dev adev;

  resource_size_t mmio_base;
  resource_size_t mmio_size;
  uint32_t __iomem *mmio;

  resource_size_t rio_mem_size;
  uint32_t __iomem *rio_mem;
};

static inline struct amd_vendor_private *adev_to_amd_private(struct amd_fake_dev *adev)
{
  return container_of(adev, struct amd_vendor_private, adev);
}

static inline struct amd_vendor_private *amd_private(struct vendor_reset_dev *vdev)
{
  return vdev->vendor_private;
}

int amd_common_probe(const struct vendor_reset_cfg *cfg, struct pci_dev *dev);
int amd_common_pre_reset(struct vendor_reset_dev *);
int amd_common_post_reset(struct vendor_reset_dev *);

int smum_send_msg_to_smc(struct amd_fake_dev *adev, uint16_t msg, uint32_t *resp);
int smum_send_msg_to_smc_with_parameter(struct amd_fake_dev *adev, uint16_t msg,
                                        uint32_t parameter, uint32_t *resp);

void amdgpu_atombios_scratch_regs_engine_hung(struct amd_fake_dev *adev, bool hung);

int smu_wait(struct amd_fake_dev *adev);
int psp_wait_for(struct amd_fake_dev *adev, uint32_t reg_index, uint32_t reg_val, uint32_t mask, bool check_changed);

#endif
