#ifndef __VENDOR_RESET_COMMON_H__
#define __VENDOR_RESET_COMMON_H__

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _H_VENDOR_RESET_DEV
#define __AMDGPU_DISCOVERY__
#define ENOTSUPP 524
#define GFP_KERNEL 0
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define DISCOVERY_TMR_SIZE (4 << 10)
#define DISCOVERY_TMR_OFFSET (64 << 10)
#define le16_to_cpu(v) (v)
#define le32_to_cpu(v) (v)
#define le64_to_cpu(v) (v)
#define kzalloc(size, flags) calloc(1, size)
#define kfree free
#define DRM_ERROR(...) ((void)0)
#define DRM_DEBUG(...) ((void)0)
#define vr_info(...) ((void)0)
#define vr_warn(...) ((void)0)
#define vr_err(...) ((void)0)

typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t loff_t;

enum {
  GC_HWIP, HDP_HWIP, SDMA0_HWIP, SDMA1_HWIP, MMHUB_HWIP,
  ATHUB_HWIP, NBIO_HWIP, MP0_HWIP, MP1_HWIP, UVD_HWIP,
  VCE_HWIP, DF_HWIP, DCE_HWIP, OSSSYS_HWIP, SMUIO_HWIP,
  PWR_HWIP, NBIF_HWIP, THM_HWIP, CLK_HWIP, MAX_HWIP
};

struct pci_dev { int unused; };
struct vendor_reset_cfg;
struct vendor_reset_dev {
  struct pci_dev *pdev;
  int reset_ret;
  void *vendor_private;
};
struct vendor_reset_ops {
  struct { unsigned int major, minor; } version;
  int (*probe)(const struct vendor_reset_cfg *, struct pci_dev *);
  int (*pre_reset)(struct vendor_reset_dev *);
  int (*reset)(struct vendor_reset_dev *);
  int (*post_reset)(struct vendor_reset_dev *);
};
struct vendor_reset_cfg {
  unsigned int vendor, device;
  const struct vendor_reset_ops *ops;
  unsigned long info;
  const char *info_str;
};
struct amd_fake_dev {
  uint32_t *reg_offset[MAX_HWIP][8];
  struct { unsigned int discovery_tmr_size; uint8_t *discovery_bin; } mman;
  struct {
    struct {
      uint32_t max_shader_engines, max_cu_per_sh, max_sh_per_se;
      uint32_t max_backends_per_se, max_texture_channel_caches, max_gprs;
      uint32_t max_gs_threads, gs_vgt_table_depth, gs_prim_buffer_depth;
      uint32_t double_offchip_lds_buf, num_sc_per_sh, num_packer_per_sc;
    } config;
    struct {
      uint32_t wave_front_size, max_waves_per_simd, max_scratch_slots_per_cu;
      uint32_t lds_size;
    } cu_info;
  } gfx;
};
struct amd_vendor_private {
  uint64_t mmio_size;
  struct amd_fake_dev adev;
};

static inline struct amd_vendor_private *amd_private(struct vendor_reset_dev *dev)
{
  return dev->vendor_private;
}

uint32_t test_read(uint32_t reg);
void test_write(uint32_t reg, uint32_t value);
#define RREG32(reg) test_read(reg)
#define WREG32(reg, value) test_write(reg, value)
int amd_common_probe(const struct vendor_reset_cfg *, struct pci_dev *);
int amd_common_pre_reset(struct vendor_reset_dev *);
int amd_common_post_reset(struct vendor_reset_dev *);
int amd_fake_dev_init(struct amd_fake_dev *, struct vendor_reset_dev *);
void amd_fake_dev_fini(struct amd_fake_dev *);
int amdgpu_discovery_get_reg_base(struct amd_fake_dev *, int, unsigned int,
                                uint32_t *, uint32_t *);
int pci_save_state(struct pci_dev *);
void pci_restore_state(struct pci_dev *);

#endif
