#include <linux/mm.h>
#include <linux/pci.h>
#include "vendor-reset-dev.h"
#include "common.h"

int amd_common_pre_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv;
  struct pci_dev *pdev = dev->pdev;

  priv = kzalloc(sizeof *priv, GFP_KERNEL);
  if (!priv)
    return -ENOMEM;

  dev->vendor_private = priv;

  spin_lock_init(&priv->pcie_lock);

  pci_set_power_state(pdev, PCI_D0);
  pci_clear_master(pdev);
  pci_save_state(pdev);
  priv->saved_state = pci_store_saved_state(pdev);
  pci_read_config_word(pdev, PCI_COMMAND, &priv->cfg);
  pci_write_config_word(pdev, PCI_COMMAND, priv->cfg | PCI_COMMAND_MEMORY | PCI_COMMAND_INTX_DISABLE);

  return 0;
}

int amd_common_post_reset(struct vendor_reset_dev *dev)
{
  struct amd_vendor_private *priv = amd_private(dev);
  struct pci_dev *pdev = dev->pdev;

  if (priv->saved_state)
  {
    pci_load_and_free_saved_state(pdev, &priv->saved_state);
    pci_restore_state(pdev);
  }
  pci_write_config_word(pdev, PCI_COMMAND, priv->cfg);

  /* don't try to go to low power if reset failed */
  if (!dev->reset_ret)
    pci_set_power_state(pdev, PCI_D3hot);

  return 0;
}
