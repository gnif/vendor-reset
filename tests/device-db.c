#include "common.h"

#define PCI_VENDOR_ID_ATI 0x1002
#include "../src/device-db.h"

const struct vendor_reset_ops amd_polaris10_ops = {0};
const struct vendor_reset_ops amd_vega10_ops = {0};
const struct vendor_reset_ops amd_vega20_ops = {0};
const struct vendor_reset_ops amd_navi10_ops = {0};

void test_device_db(void)
{
  unsigned int i, matches = 0;

  for (i = 0; vendor_reset_devices[i].vendor; ++i)
  {
    const struct vendor_reset_cfg *cfg = &vendor_reset_devices[i];

    if (cfg->device != 0x7550)
      continue;
    ++matches;
    assert(cfg->vendor == 0x1002);
    assert(cfg->info == AMD_NAVI48);
    assert(strcmp(cfg->info_str, "AMD_NAVI48") == 0);
    assert(cfg->ops == &amd_navi48_ops);
  }
  assert(matches == 1);
}
