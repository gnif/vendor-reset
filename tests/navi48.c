#include "common.h"

/* Discovery defines these registers itself and must not hide missing includes. */
#include "../src/amd/navi48.c"
_Static_assert(mmMM_INDEX == 0x0, "Navi48 needs the MMIO index definition");
_Static_assert(mmMM_DATA == 0x1, "Navi48 needs the MMIO data definition");
#include "../src/amd/amdgpu/amdgpu_discovery.c"

static uint8_t image[10 << 10];
static struct amd_vendor_private priv;
static struct pci_dev pdev;
static struct vendor_reset_dev dev = { .pdev = &pdev, .vendor_private = &priv };
static bool boot_ready, memory_ready, mailbox_accessible;
static uint32_t memsize;
static unsigned int writes, saves, restores, cleanups, sleep_ms, reads;
static int save_ret, post_ret;
static uint32_t write_regs[3], write_values[3];

#define SMU_BASE 0x17000
#define PSP_BASE 0x16000

uint32_t test_read(uint32_t reg)
{
  ++reads;
  if (reg == NAVI48_CONFIG_MEMSIZE)
    return writes && !memory_ready ? 0xffffffff : memsize;
  if (reg == SMU_BASE + NAVI48_DEBUG_RESP)
    return mailbox_accessible ? 1 : 0xffffffff;
  if (reg == PSP_BASE + NAVI48_PSP_BL_READY)
    return boot_ready ? 0x80000000 : 0xffffffff;
  assert(!"unexpected register read");
  return 0;
}

void test_write(uint32_t reg, uint32_t value)
{
  assert(writes < ARRAY_SIZE(write_regs));
  write_regs[writes] = reg;
  write_values[writes++] = value;
}

void amdgpu_device_vram_access(struct amd_fake_dev *adev, loff_t pos,
                              uint32_t *buf, size_t size, bool write)
{
  assert(!write);
  assert(pos == ((int64_t)16384 << 20) - DISCOVERY_TMR_OFFSET);
  assert(size <= sizeof(image));
  memcpy(buf, image, size);
}

void msleep(unsigned int milliseconds) { sleep_ms += milliseconds; }
void usleep_range(unsigned long min, unsigned long max) { sleep_ms += min / 1000; }
int pci_save_state(struct pci_dev *pci) { ++saves; return save_ret; }
void pci_restore_state(struct pci_dev *pci) { ++restores; }
int amd_common_probe(const struct vendor_reset_cfg *cfg, struct pci_dev *pci) { return 0; }
int amd_common_pre_reset(struct vendor_reset_dev *vdev) { return 0; }
int amd_common_post_reset(struct vendor_reset_dev *vdev) { return post_ret; }
int amd_fake_dev_init(struct amd_fake_dev *adev, struct vendor_reset_dev *vdev) { return 0; }
void amd_fake_dev_fini(struct amd_fake_dev *adev)
{
  ++cleanups;
  amdgpu_discovery_fini(adev);
}

static struct ip_discovery_header *discovery(void)
{
  struct binary_header *bhdr = (struct binary_header *)image;
  return (struct ip_discovery_header *)(image + bhdr->table_list[IP_DISCOVERY].offset);
}

static struct ip *first_ip(void)
{
  return (struct ip *)(image + discovery()->die_info[0].die_offset +
                       sizeof(struct die_header));
}

static void checksums(void)
{
  struct binary_header *bhdr = (struct binary_header *)image;
  struct table_info *info;
  size_t offset = offsetof(struct binary_header, binary_checksum) + sizeof(uint16_t);
  int i;

  for (i = IP_DISCOVERY; i <= GC; ++i)
  {
    info = &bhdr->table_list[i];
    info->checksum = amdgpu_discovery_calculate_checksum(image + info->offset, info->size);
  }
  bhdr->binary_checksum = amdgpu_discovery_calculate_checksum(
      image + offset, bhdr->binary_size - offset);
}

static void fixture(bool addresses_64)
{
  struct binary_header *bhdr;
  struct ip_discovery_header *ihdr;
  struct die_header *dhdr;
  struct ip *ip;
  struct gpu_info_header *ghdr;
  size_t offset, stride = addresses_64 ? 8 : 4;
  uint64_t address;

  assert(!priv.adev.mman.discovery_bin);
  memset(&priv, 0, sizeof(priv));
  memset(image, 0, sizeof(image));
  priv.mmio_size = 0x100000;
  boot_ready = memory_ready = mailbox_accessible = true;
  memsize = 16384;
  writes = saves = restores = cleanups = sleep_ms = reads = 0;
  save_ret = post_ret = dev.reset_ret = 0;

  bhdr = (struct binary_header *)image;
  bhdr->binary_signature = BINARY_SIGNATURE;
  bhdr->table_list[IP_DISCOVERY].offset = sizeof(*bhdr);
  ihdr = discovery();
  ihdr->signature = DISCOVERY_TABLE_SIGNATURE;
  ihdr->version = addresses_64 ? 4 : 3;
  ihdr->padding[0] = addresses_64;
  ihdr->num_dies = 1;
  offset = sizeof(*bhdr) + sizeof(*ihdr);
  ihdr->die_info[0].die_offset = offset;
  dhdr = (struct die_header *)(image + offset);
  dhdr->num_ips = 2;
  offset += sizeof(*dhdr);
  ip = (struct ip *)(image + offset);
  ip->hw_id = MP1_HWID;
  ip->num_base_address = 2;
  ip->major = 14;
  ip->revision = 2;
  if (addresses_64)
  {
    address = 0x12340000c0000000ULL | SMU_BASE;
    memcpy((uint8_t *)ip + offsetof(struct ip, base_address) + stride, &address, stride);
  }
  else
    ip->base_address[1] = SMU_BASE;
  offset += offsetof(struct ip, base_address) + 2 * stride;
  ip = (struct ip *)(image + offset);
  ip->hw_id = MP0_HWID;
  ip->num_base_address = 1;
  ip->major = 14;
  ip->revision = 3;
  address = PSP_BASE;
  memcpy((uint8_t *)ip + offsetof(struct ip, base_address), &address, stride);
  offset += offsetof(struct ip, base_address) + stride;
  ihdr->size = offset - sizeof(*bhdr);
  bhdr->table_list[IP_DISCOVERY].size = ihdr->size;
  bhdr->table_list[GC].offset = offset;
  bhdr->table_list[GC].size = sizeof(*ghdr);
  ghdr = (struct gpu_info_header *)(image + offset);
  ghdr->size = sizeof(*ghdr);
  bhdr->binary_size = offset + sizeof(*ghdr);
  checksums();
}

static int reset(void)
{
  int ret = amd_navi48_ops.reset(&dev);
  assert(!priv.adev.mman.discovery_bin);
  dev.reset_ret = ret;
  return amd_navi48_ops.post_reset(&dev);
}

static void test_success(bool addresses_64)
{
  fixture(addresses_64);
  assert(reset() == 0);
  assert(writes == 3 && saves == 1 && restores == 1 && cleanups == 1);
  assert(write_regs[0] == SMU_BASE + NAVI48_DEBUG_PARAM && write_values[0] == 0);
  assert(write_regs[1] == SMU_BASE + NAVI48_DEBUG_MSG && write_values[1] == 2);
  assert(write_regs[2] == SMU_BASE + NAVI48_DEBUG_RESP && write_values[2] == 0);
  assert(sleep_ms == 1000);
}

static void test_failures(void)
{
  struct binary_header *bhdr;

  fixture(false);
  boot_ready = false;
  assert(reset() == -ETIMEDOUT);
  assert(sleep_ms == 2000 && reads < 1100);

  fixture(false);
  memory_ready = false;
  assert(reset() == -ETIMEDOUT);
  assert(sleep_ms == 2000 && reads < 1100);

  fixture(false);
  mailbox_accessible = false;
  assert(reset() == -EIO && writes == 0);

  fixture(false);
  save_ret = -ENOMEM;
  assert(reset() == -ENOMEM && writes == 0);

  fixture(false);
  post_ret = -EIO;
  assert(reset() == -EIO);

  fixture(false);
  memsize = 0;
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  memsize = 0xffffffff;
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  first_ip()->major = 11;
  checksums();
  assert(reset() == -ENOTSUPP && writes == 0);

  fixture(false);
  ((struct ip *)((uint8_t *)first_ip() + offsetof(struct ip, base_address) + 8))->major = 11;
  checksums();
  assert(reset() == -ENOTSUPP && writes == 0);

  fixture(false);
  priv.mmio_size = (NAVI48_CONFIG_MEMSIZE + 1) * 4 - 1;
  assert(reset() == -EINVAL && reads == 0 && writes == 0);

  fixture(false);
  priv.mmio_size = (SMU_BASE + NAVI48_DEBUG_MSG + 1) * 4 - 1;
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  first_ip()->base_address[1] = 0xffffffff;
  checksums();
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  first_ip()->num_base_address = 1;
  checksums();
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  first_ip()->num_base_address = 0;
  checksums();
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  discovery()->version = 5;
  checksums();
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  discovery()->num_dies = 17;
  checksums();
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  discovery()->die_info[0].die_offset = 0xffff;
  checksums();
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  first_ip()->num_base_address = 255;
  checksums();
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  first_ip()->hw_id = MP2_HWID;
  checksums();
  assert(reset() == -ENODEV && writes == 0);

  fixture(false);
  image[0] ^= 1;
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  image[sizeof(struct binary_header)] ^= 1;
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  bhdr = (struct binary_header *)image;
  bhdr->table_list[IP_DISCOVERY].offset = 0xffff;
  bhdr->binary_checksum = amdgpu_discovery_calculate_checksum(
      image + 10, bhdr->binary_size - 10);
  assert(reset() == -EINVAL && writes == 0);

  fixture(false);
  bhdr = (struct binary_header *)image;
  bhdr->binary_size = sizeof(image) + 1;
  assert(reset() == -EINVAL && writes == 0);
}

static void test_legacy_discovery(void)
{
  int major, minor, revision;

  fixture(false);
  assert(amdgpu_discovery_reg_base_init(&priv.adev) == 0);
  assert(priv.adev.mman.discovery_tmr_size == DISCOVERY_TMR_SIZE);
  assert(priv.adev.reg_offset[MP1_HWIP][0][1] == SMU_BASE);
  assert(priv.adev.reg_offset[MP0_HWIP][0][0] == PSP_BASE);
  assert(amdgpu_discovery_get_ip_version(&priv.adev, MP1_HWID,
                                        &major, &minor, &revision) == 0);
  assert(major == 14 && minor == 0 && revision == 2);
  amd_fake_dev_fini(&priv.adev);
}

static void test_discovery_mutations(void)
{
  unsigned int i;
  uint32_t base, version;

  for (i = 0; i < 512; ++i)
  {
    size_t offset;
    int ret;

    fixture(i & 1);
    offset = sizeof(struct binary_header) + (i / 8) % sizeof(struct ip_discovery_header);
    image[offset] ^= 1U << (i % 8);
    checksums();
    ret = amdgpu_discovery_get_reg_base(&priv.adev, MP1_HWID, 1, &base, &version);
    assert(ret == 0 || ret == -EINVAL || ret == -ENODEV);
    amd_fake_dev_fini(&priv.adev);
  }
}

void test_device_db(void);

int main(void)
{
  test_device_db();
  test_success(false);
  test_success(true);
  test_failures();
  test_legacy_discovery();
  test_discovery_mutations();
  puts("Navi48 reset and discovery tests passed");
  return 0;
}
