#ifndef __OF_PCI_H
#define __OF_PCI_H

#include <linux/pci.h>

struct pci_dev;
struct of_phandle_args;
int of_irq_parse_pci(const struct pci_dev *pdev, struct of_phandle_args *out_irq);

struct device_node;
struct device_node *of_pci_find_child_device(struct device_node *parent,
					     unsigned int devfn);

#endif
