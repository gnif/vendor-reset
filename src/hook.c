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

#include "hook.h"
#include "ftrace.h"
#include "vendor-reset-dev.h"

#include <linux/module.h>

static bool install_hook = true;
module_param(install_hook, bool, 0);

static bool hook_installed = false;
static int (*orig_pci_dev_specific_reset)(struct pci_dev *dev, int probe);

static int hooked_pci_dev_specific_reset(struct pci_dev *dev, int probe)
{
  int ret;
  struct vendor_reset_cfg *cfg;

  cfg = vendor_reset_cfg_find(dev->vendor, dev->device);
  if (!cfg)
    goto do_orig;

  if (probe)
    return cfg->ops->probe(cfg, dev);

  ret = vendor_reset_dev_locked(cfg, dev);
  if (!ret || ret != -ENOTTY)
    return ret;

do_orig:
  return orig_pci_dev_specific_reset(dev, probe);
}

struct ftrace_hook fh_hooks[] = {
  HOOK("pci_dev_specific_reset", &orig_pci_dev_specific_reset, hooked_pci_dev_specific_reset),
  {0},
};

long vendor_reset_hook_init(void)
{
  int ret = 0;

  if (install_hook)
  {
     ret = fh_install_hooks(fh_hooks);
     if (ret)
       pr_warn("vendor_reset_hook: install failed");
     else
     {
       pr_info("vendor_reset_hook: installed");
       hook_installed = true;
     }
  }

  return ret;
}

void vendor_reset_hook_exit(void)
{
  if (hook_installed)
  {
    fh_remove_hooks(fh_hooks);
    hook_installed = false;
  }
}
