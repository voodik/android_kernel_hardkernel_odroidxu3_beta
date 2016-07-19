/*
 * ODROID GPIO memory control driver (/dev/gpiomem)
 *
 * Copyright (C) 2017, Hardkernel Co,.Ltd
 * Author: Charles Park <charles.park@hardkernel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
/*---------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/pagemap.h>
#include <linux/io.h>

/*---------------------------------------------------------------------------*/
#define	ODROIDC2_GPIO_BASE	0xC8834000

#define DEVICE_NAME	"odroid-aml_s905"
#define DRIVER_NAME	"odroid-gpiomem"
#define DEVICE_MINOR	0

struct odroid_gpiomem {
	unsigned long	regs_phys;
	struct device	*dev;
};

static struct cdev		gpiomem_cdev;
static dev_t			gpiomem_devid;
static struct class		*gpiomem_class;
static struct device		*gpiomem_dev;
static struct odroid_gpiomem	*gpiomem;

/*---------------------------------------------------------------------------*/
static int gpiomem_open(struct inode *inode, struct file *file)
{
	int dev = iminor(inode);
	int ret = 0;

	dev_info(gpiomem->dev, "gpiomem device opened.");

	if (dev != DEVICE_MINOR) {
		dev_err(gpiomem->dev, "Unknown minor device: %d", dev);
		ret = -ENXIO;
	}
	return ret;
}

/*---------------------------------------------------------------------------*/
static int gpiomem_release(struct inode *inode, struct file *file)
{
	int dev = iminor(inode);
	int ret = 0;

	if (dev != DEVICE_MINOR) {
		dev_err(gpiomem->dev, "Unknown minor device %d", dev);
		ret = -ENXIO;
	}
	return ret;
}

/*---------------------------------------------------------------------------*/
static const struct vm_operations_struct gpiomem_vm_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

/*---------------------------------------------------------------------------*/
static int gpiomem_mmap(struct file *file, struct vm_area_struct *vma)
{
	/* Ignore what the user says - they're getting the GPIO regs
	   whether they like it or not! */
	unsigned long gpio_page = gpiomem->regs_phys >> PAGE_SHIFT;

	vma->vm_page_prot = phys_mem_access_prot(file, gpio_page,
						 PAGE_SIZE,
						 vma->vm_page_prot);
	vma->vm_ops = &gpiomem_vm_ops;
	if (remap_pfn_range(vma, vma->vm_start,
			gpio_page,
			PAGE_SIZE,
			vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

/*---------------------------------------------------------------------------*/
static const struct file_operations gpiomem_fops = {
	.owner	 = THIS_MODULE,
	.open	 = gpiomem_open,
	.release = gpiomem_release,
	.mmap	 = gpiomem_mmap,
};


/*---------------------------------------------------------------------------*/
static int odroid_gpiomem_probe(struct platform_device *pdev)
{
	int err;
	void *ptr_err;
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct resource *ioresource;

	/* Allocate buffers and gpiomem data */
	gpiomem = kzalloc(sizeof(struct odroid_gpiomem), GFP_KERNEL);

	if (!gpiomem) {
		err = -ENOMEM;
		goto failed_alloc;
	}

	gpiomem->dev = dev;

	/* Create character device entries */

	err = alloc_chrdev_region(&gpiomem_devid,
				  DEVICE_MINOR, 1, DEVICE_NAME);
	if (err != 0) {
		dev_err(gpiomem->dev, "unable to allocate device number");
		goto failed_alloc_chrdev;
	}

	cdev_init(&gpiomem_cdev, &gpiomem_fops);
	gpiomem_cdev.owner = THIS_MODULE;
	err = cdev_add(&gpiomem_cdev, gpiomem_devid, 1);
	if (err != 0) {
		dev_err(gpiomem->dev, "unable to register device");
		goto failed_cdev_add;
	}

	/* Create sysfs entries */
	gpiomem_class = class_create(THIS_MODULE, DEVICE_NAME);
	ptr_err = gpiomem_class;
	if (IS_ERR(ptr_err))
		goto failed_class_create;

	gpiomem_dev = device_create(gpiomem_class, NULL,
					gpiomem_devid, NULL,
					"gpiomem");
	ptr_err = gpiomem_dev;
	if (IS_ERR(ptr_err))
		goto failed_device_create;

	if (node) {
		ioresource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		gpiomem->regs_phys = ioresource->start;
	} else {
		gpiomem->regs_phys = ODROIDC2_GPIO_BASE;
	}

	dev_info(gpiomem->dev, "Initialised: Registers at 0x%08lx",
		gpiomem->regs_phys);

	return 0;

failed_device_create:
	class_destroy(gpiomem_class);
failed_class_create:
	cdev_del(&gpiomem_cdev);
	err = PTR_ERR(ptr_err);
failed_cdev_add:
	unregister_chrdev_region(gpiomem_devid, 1);
failed_alloc_chrdev:
	kfree(gpiomem);
failed_alloc:
	dev_err(gpiomem->dev, "could not load bcm2835_gpiomem");
	return err;
}

/*---------------------------------------------------------------------------*/
static int odroid_gpiomem_remove(struct platform_device *pdev)
{
	struct device *dev = gpiomem->dev;

	kfree(gpiomem);
	device_destroy(gpiomem_class, gpiomem_devid);
	class_destroy(gpiomem_class);
	cdev_del(&gpiomem_cdev);
	unregister_chrdev_region(gpiomem_devid, 1);

	dev_info(dev, "GPIO mem driver removed - OK");
	return 0;
}

/*---------------------------------------------------------------------------*/
static const struct of_device_id odroid_gpiomem_dt[] = {
	{.compatible = "odroid-gpiomem",},
	{ /* sentinel */ },
};

MODULE_DEVICE_TABLE(of, odroid_gpiomem_dt);

static struct platform_driver odroid_gpiomem_driver = {
	.probe  = odroid_gpiomem_probe,
	.remove = odroid_gpiomem_remove,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = odroid_gpiomem_dt,
	},
};

/*---------------------------------------------------------------------------*/
module_platform_driver(odroid_gpiomem_driver);

MODULE_ALIAS("platform:odroid-aml_s905");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("gpiomem driver for accessing GPIO from userspace");
MODULE_AUTHOR("HARDKERNEL Co., Ltd");
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
