/*
 * Media device
 *
 * Copyright (C) 2010 Nokia Corporation
 *
 * Contacts: Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *	     Sakari Ailus <sakari.ailus@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MEDIA_DEVICE_H
#define _MEDIA_DEVICE_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>

#include <media/media-devnode.h>
#include <media/media-entity.h>

struct device;

/**
 * struct media_device - Media device
 * @dev:	Parent device
 * @devnode:	Media device node
 * @model:	Device model name
 * @serial:	Device serial number (optional)
 * @bus_info:	Unique and stable device location identifier
 * @hw_revision: Hardware device revision
 * @driver_version: Device driver version
 * @topology_version: Monotonic counter for storing the version of the graph
 *		topology. Should be incremented each time the topology changes.
 * @entity_id:	Unique ID used on the last entity registered
 * @pad_id:	Unique ID used on the last pad registered
 * @link_id:	Unique ID used on the last link registered
 * @intf_devnode_id: Unique ID used on the last interface devnode registered
 * @entities:	List of registered entities
 * @interfaces:	List of registered interfaces
 * @pads:	List of registered pads
 * @links:	List of registered links
 * @lock:	Entities list lock
 * @graph_mutex: Entities graph operation lock
 * @link_notify: Link state change notification callback
 *
 * This structure represents an abstract high-level media device. It allows easy
 * access to entities and provides basic media device-level support. The
 * structure can be allocated directly or embedded in a larger structure.
 *
 * The parent @dev is a physical device. It must be set before registering the
 * media device.
 *
 * @model is a descriptive model name exported through sysfs. It doesn't have to
 * be unique.
 */
struct media_device {
	/* dev->driver_data points to this struct. */
	struct device *dev;
	struct media_devnode devnode;

	char model[32];
	char serial[40];
	char bus_info[32];
	u32 hw_revision;
	u32 driver_version;

	u32 topology_version;

	u32 entity_id;
	u32 pad_id;
	u32 link_id;
	u32 intf_devnode_id;

	struct list_head entities;
	struct list_head interfaces;
	struct list_head pads;
	struct list_head links;

	/* Protects the graph objects creation/removal */
	spinlock_t lock;
	/* Serializes graph operations. */
	struct mutex graph_mutex;

	int (*link_notify)(struct media_link *link, u32 flags,
			   unsigned int notification);
};

#ifdef CONFIG_MEDIA_CONTROLLER

/* Supported link_notify @notification values. */
#define MEDIA_DEV_NOTIFY_PRE_LINK_CH	0
#define MEDIA_DEV_NOTIFY_POST_LINK_CH	1

/* media_devnode to media_device */
#define to_media_device(node) container_of(node, struct media_device, devnode)

/**
 * media_device_init() - Initializes a media device element
 *
 * @mdev:	pointer to struct &media_device
 *
 * This function initializes the media device prior to its registration.
 * The media device initialization and registration is split in two functions
 * to avoid race conditions and make the media device available to user-space
 * before the media graph has been completed.
 *
 * So drivers need to first initialize the media device, register any entity
 * within the media device, create pad to pad links and then finally register
 * the media device by calling media_device_register() as a final step.
 */
void media_device_init(struct media_device *mdev);

/**
 * media_device_cleanup() - Cleanups a media device element
 *
 * @mdev:	pointer to struct &media_device
 *
 * This function that will destroy the graph_mutex that is
 * initialized in media_device_init().
 */
void media_device_cleanup(struct media_device *mdev);

/**
 * __media_device_register() - Registers a media device element
 *
 * @mdev:	pointer to struct &media_device
 * @owner:	should be filled with %THIS_MODULE
 *
 * Users, should, instead, call the media_device_register() macro.
 *
 * The caller is responsible for initializing the media_device structure before
 * registration. The following fields must be set:
 *
 *  - dev must point to the parent device (usually a &pci_dev, &usb_interface or
 *    &platform_device instance).
 *
 *  - model must be filled with the device model name as a NUL-terminated UTF-8
 *    string. The device/model revision must not be stored in this field.
 *
 * The following fields are optional:
 *
 *  - serial is a unique serial number stored as a NUL-terminated ASCII string.
 *    The field is big enough to store a GUID in text form. If the hardware
 *    doesn't provide a unique serial number this field must be left empty.
 *
 *  - bus_info represents the location of the device in the system as a
 *    NUL-terminated ASCII string. For PCI/PCIe devices bus_info must be set to
 *    "PCI:" (or "PCIe:") followed by the value of pci_name(). For USB devices,
 *    the usb_make_path() function must be used. This field is used by
 *    applications to distinguish between otherwise identical devices that don't
 *    provide a serial number.
 *
 *  - hw_revision is the hardware device revision in a driver-specific format.
 *    When possible the revision should be formatted with the KERNEL_VERSION
 *    macro.
 *
 *  - driver_version is formatted with the KERNEL_VERSION macro. The version
 *    minor must be incremented when new features are added to the userspace API
 *    without breaking binary compatibility. The version major must be
 *    incremented when binary compatibility is broken.
 *
 * Notes:
 *
 * Upon successful registration a character device named media[0-9]+ is created.
 * The device major and minor numbers are dynamic. The model name is exported as
 * a sysfs attribute.
 *
 * Unregistering a media device that hasn't been registered is *NOT* safe.
 */
int __must_check __media_device_register(struct media_device *mdev,
					 struct module *owner);
#define media_device_register(mdev) __media_device_register(mdev, THIS_MODULE)
void media_device_unregister(struct media_device *mdev);

int __must_check media_device_register_entity(struct media_device *mdev,
					      struct media_entity *entity);
void media_device_unregister_entity(struct media_entity *entity);

/**
 * media_device_get_devres() -	get media device as device resource
 *				creates if one doesn't exist
 *
 * @dev: pointer to struct &device.
 *
 * Sometimes, the media controller &media_device needs to be shared by more
 * than one driver. This function adds support for that, by dynamically
 * allocating the &media_device and allowing it to be obtained from the
 * struct &device associated with the common device where all sub-device
 * components belong. So, for example, on an USB device with multiple
 * interfaces, each interface may be handled by a separate per-interface
 * drivers. While each interface have its own &device, they all share a
 * common &device associated with the hole USB device.
 */
struct media_device *media_device_get_devres(struct device *dev);

/**
 * media_device_find_devres() - find media device as device resource
 *
 * @dev: pointer to struct &device.
 */
struct media_device *media_device_find_devres(struct device *dev);

/* Iterate over all entities. */
#define media_device_for_each_entity(entity, mdev)			\
	list_for_each_entry(entity, &(mdev)->entities, graph_obj.list)

/* Iterate over all interfaces. */
#define media_device_for_each_intf(intf, mdev)			\
	list_for_each_entry(intf, &(mdev)->interfaces, graph_obj.list)

/* Iterate over all pads. */
#define media_device_for_each_pad(pad, mdev)			\
	list_for_each_entry(pad, &(mdev)->pads, graph_obj.list)

/* Iterate over all links. */
#define media_device_for_each_link(link, mdev)			\
	list_for_each_entry(link, &(mdev)->links, graph_obj.list)


#else
static inline int media_device_register(struct media_device *mdev)
{
	return 0;
}
static inline void media_device_unregister(struct media_device *mdev)
{
}
static inline int media_device_register_entity(struct media_device *mdev,
						struct media_entity *entity)
{
	return 0;
}
static inline void media_device_unregister_entity(struct media_entity *entity)
{
}
static inline struct media_device *media_device_get_devres(struct device *dev)
{
	return NULL;
}
static inline struct media_device *media_device_find_devres(struct device *dev)
{
	return NULL;
}
#endif /* CONFIG_MEDIA_CONTROLLER */
#endif
