//[*]-------------------------------------------------------------------------[*]
//
// ODROID USBIO + Accelerometer Sensor Driver (USB HID Protocol)
// Hardkernel : 2015/11/09 Charles.park
//
//[*]-------------------------------------------------------------------------[*]
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/input/mt.h>

//[*]-------------------------------------------------------------------------[*]
#include "odroid-usbio.h"

//[*]-------------------------------------------------------------------------[*]
/* usbio button map data */
struct button BUTTON[NUM_OF_KEYS] = {{0 ,}, };

/* Acc Sensor Mount location */
unsigned long Orientation = 0;

//[*]-------------------------------------------------------------------------[*]
/***************************************************************************

   boot.ini parse description

   base rule :
       - button number, button type, button keycode or button touch x, y -

   bootargs setup ex:
   
       - button_map="0:K:128,1:T:100:200,2:M:U"

   description :
       button id = 0, key, keycode=128 (keycode refer uapi/linux/input.h)
       button id = 1, touch button, x=100, y=200
       button id = 2, mouse whell up
       
   config string :
       button type : 'K'
           args : 'button code,
       button type : 'M'
           args : 'U/D/L/R' -> mouse direction
                  'l/m/r'   -> mouse button
       button type : 'T'
           args : 'location x, y' -> button x, y

****************************************************************************/
//[*]-------------------------------------------------------------------------[*]
static int __init orientation_setup(char *line)
{
	if(kstrtoul(line, 10, &Orientation) != 0)    Orientation = 0;

	return	0;
}
__setup("acc_orientation=", orientation_setup);

//[*]-------------------------------------------------------------------------[*]
static int __init parse_key(char *str, struct button *bt)
{
	char *item = NULL;

	if ((item = strsep(&str, ":")) != NULL)	{
		if (kstrtoul(item, 10, &bt->code) != 0)	return 0;

		if (bt->code >= KEY_MAX) return 0;
		return 1;
	}
	return 0;
}

//[*]-------------------------------------------------------------------------[*]
static int __init parse_mouse(char *str, struct button *bt)
{
	char *item = NULL;

	if ((item = strsep(&str, ":")) != NULL) {
		switch(item[0]) {
			case 'U': bt->code = KEY_UP;
				  break;
			case 'L': bt->code = KEY_LEFT;
				  break;
			case 'D': bt->code = KEY_DOWN;
				  break;
			case 'R': bt->code = KEY_RIGHT;
				  break;

			case 'l': bt->code = BTN_LEFT;
				  break;
			case 'r': bt->code = BTN_RIGHT;
				  break;
			case 'm': bt->code = BTN_MIDDLE;
				  break;

			default : return 0;
		}
		return 1;
	}
	return 0;
}

//[*]-------------------------------------------------------------------------[*]
static int __init parse_touch(char *str, struct button *bt)
{
	char *item = NULL;

	if ((item = strsep(&str, ":")) != NULL) {
		if(kstrtoul(item, 10, &bt->x) != 0) return 0;
	}

	if ((item = strsep(&str, ":")) != NULL) {
		if(kstrtoul(item, 10, &bt->y) != 0) return 0;
		return 1;
	}
	return 0;
}

//[*]-------------------------------------------------------------------------[*]
static int __init parse_bootargs(char *line)
{
	char		*item = NULL, *str = NULL;
	unsigned long	key_num;

	// parse bootargs
	while (line != NULL) {
		if ((str = strsep(&line, ",")) == NULL)	return 0;

		if ((item = strsep(&str, ":")) == NULL) return 0;

		if (kstrtoul(item, 10, &key_num) != 0) 	return 0;

		if (key_num >= NUM_OF_KEYS)		return 0;

		BUTTON[key_num].use = true;

		item = strsep(&str, ":");
		switch(item[0]) {
			case 'K': BUTTON[key_num].type = BUTTON_TYPE_KEY;
				  BUTTON[key_num].use =
					parse_key(str, &BUTTON[key_num]);
				  break;

			case 'M': BUTTON[key_num].type = BUTTON_TYPE_MOUSE;
				  BUTTON[key_num].use =
				  	parse_mouse(str, &BUTTON[key_num]);
				  break;

			case 'T': BUTTON[key_num].type = BUTTON_TYPE_TOUCH;
				  BUTTON[key_num].use =
					parse_touch(str, &BUTTON[key_num]);
				  break;

			default : BUTTON[key_num].use = false;
		}
	}
	return 0;
}

__setup("button_map=", parse_bootargs);

//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
static void usbio_raw_process (struct usbio *usbio,
				unsigned char *pkt, int len)
{
	struct  usbio_raw	*usbio_raw = (struct usbio_raw *)pkt;

	if ((usbio_raw->header == USBIO_HEADER_PROTOCOL) &&
	    (usbio_raw->tail   == USBIO_TAIL_PROTOCOL  )) {
	    	usbio_sensor_process(usbio, pkt);
		usbio_keypad_process(usbio, usbio_raw->key);
		usbio_mouse_process (usbio, usbio_raw->key);
	}
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_irq (struct urb *urb)
{
	struct usbio *usbio = urb->context;
	struct device *dev = &usbio->interface->dev;
	int retval;

	switch (urb->status) {
		case 0:
		/* success */
		break;
		case -ETIME:
			/* this urb is timing out */
			dev_dbg(dev, "%s - urb timed out - was the device unplugged?\n", __func__);
			return;
		case -ECONNRESET:
		case -ENOENT:
		case -ESHUTDOWN:
		case -EPIPE:
			/* this urb is terminated, clean up */
			dev_dbg(dev, "%s - urb shutting down with status: %d\n", __func__, urb->status);
			return;
		default:
			dev_dbg(dev, "%s - nonzero urb status received: %d\n", __func__, urb->status);
			goto exit;
	}

	usbio_raw_process(usbio, usbio->data, urb->actual_length);
exit:
	usb_mark_last_busy(interface_to_usbdev(usbio->interface));
	if ((retval = usb_submit_urb(urb, GFP_ATOMIC))) {
		dev_err(dev, "%s - usb_submit_urb failed with result: %d\n", __func__, retval);
	}
}

//[*]-------------------------------------------------------------------------[*]
static int usbio_suspend  (struct usb_interface *intf, pm_message_t message)
{
	struct usbio *usbio = usb_get_intfdata(intf);

	usbio->sensor->on_before_suspend = usbio->sensor->enabled;
	usbio_sensor_disable(usbio);
	usb_kill_urb(usbio->irq);

	return 0;
}

//[*]-------------------------------------------------------------------------[*]
static int usbio_resume   (struct usb_interface *intf)
{
	struct usbio *usbio = usb_get_intfdata(intf);
	struct input_dev *input = usbio->sensor->input;
	int result = 0;

	mutex_lock(&input->mutex);
	if (input->users)
		result = usb_submit_urb(usbio->irq, GFP_NOIO);
	mutex_unlock(&input->mutex);
	
	if (usbio->sensor->on_before_suspend)
		usbio_sensor_enable(usbio);

	return result;
}

//[*]-------------------------------------------------------------------------[*]
static int usbio_reset_resume   (struct usb_interface *intf)
{
	struct usbio *usbio = usb_get_intfdata(intf);
	struct input_dev *input = usbio->sensor->input;
	int err = 0;

	/* restart IO if needed */
	mutex_lock(&input->mutex);
	if (input->users)
		err = usb_submit_urb(usbio->irq, GFP_NOIO);
	mutex_unlock(&input->mutex);

	if (usbio->sensor->on_before_suspend)
		usbio_sensor_enable(usbio);

	return err;
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_free_buffers(struct usb_device *udev,
				struct usbio *usbio)
{
	usb_free_coherent(udev, usbio->data_size, usbio->data, usbio->data_dma);
}

//[*]-------------------------------------------------------------------------[*]
static struct usb_endpoint_descriptor *
	usbio_sensor_get_input_endpoint(struct usb_host_interface *interface)
{
	int i;

	for (i = 0; i < interface->desc.bNumEndpoints; i++) {
		if (usb_endpoint_dir_in(&interface->endpoint[i].desc))
			return &interface->endpoint[i].desc;
	}

	return NULL;
}

//[*]-------------------------------------------------------------------------[*]
static int usbio_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_endpoint_descriptor *endpoint;
	struct usbio *usbio = NULL;

	int err = 0;

	if (!(endpoint = usbio_sensor_get_input_endpoint(intf->cur_altsetting)))
		return	-ENXIO;

	if (!(usbio = devm_kzalloc(&intf->dev, sizeof(struct usbio), GFP_KERNEL)))
		return	-ENOMEM;

	if (!(usbio->sensor = devm_kzalloc(&intf->dev, sizeof(struct sensor), GFP_KERNEL)))
		return	-ENOMEM;

	if (!(usbio->keypad = devm_kzalloc(&intf->dev, sizeof(struct keypad), GFP_KERNEL)))
		return	-ENOMEM;

	if (!(usbio->mouse = devm_kzalloc(&intf->dev, sizeof(struct mouse), GFP_KERNEL)))
		return	-ENOMEM;

	if (!(usbio->sensor->input = input_allocate_device()))
		return	-ENOMEM;

	if (!(usbio->keypad->input = input_allocate_device()))
		goto	err_free_mem;

	if (!(usbio->mouse->input = input_allocate_device()))
		goto	err_free_mem;

	usbio->data_size = sizeof(struct usbio_raw);
	usbio->data	 = usb_alloc_coherent(udev, usbio->data_size,
						GFP_KERNEL, &usbio->data_dma);
	if (!usbio->data)
		goto err_free_mem;

	if (!(usbio->irq = usb_alloc_urb(0, GFP_KERNEL)))	{
		dev_dbg(&intf->dev, "%s - usb_alloc_urb failed: usbtouch->irq\n", __func__);
		goto err_free_buffers;
	}

	if (usb_endpoint_type(endpoint) == USB_ENDPOINT_XFER_INT)	{
		usb_fill_int_urb(usbio->irq, udev,
			usb_rcvintpipe(udev, endpoint->bEndpointAddress),
			usbio->data, usbio->data_size,
			usbio_irq, usbio, endpoint->bInterval);
	}
	else	{
		usb_fill_bulk_urb(usbio->irq, udev,
			usb_rcvbulkpipe(udev, endpoint->bEndpointAddress),
			usbio->data, usbio->data_size,
			usbio_irq, usbio);
	}

	usbio->irq->dev = udev;
	usbio->irq->transfer_dma = usbio->data_dma;
	usbio->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	usbio->interface = intf;

	if (udev->manufacturer) {
		strlcpy(usbio->name, udev->manufacturer, sizeof(usbio->name));
	}

	if (udev->product) {
		if (udev->manufacturer) strlcat(usbio->name, " ", sizeof(usbio->name));

		strlcat(usbio->name, udev->product, sizeof(usbio->name));
	}

	if (!strlen(usbio->name)) {
		snprintf(usbio->name, sizeof(usbio->name),
				"ODROID USBIO Sensor Board %04x:%04x",
				le16_to_cpu(udev->descriptor.idVendor),
				le16_to_cpu(udev->descriptor.idProduct));
	}

	usb_make_path(udev, usbio->phys, sizeof(usbio->phys));
	strlcat(usbio->phys, "/input0", sizeof(usbio->phys));

	usb_to_input_id(udev, &usbio->sensor->input->id);

	usbio->sensor->input->dev.parent = &intf->dev;

	if ((err = usbio_sensor_init (usbio)))
		goto	err_free_urb;

	if ((err = usbio_keypad_init (usbio)))
		goto	err_free_urb;

	if ((err = usbio_mouse_init (usbio)))
		goto	err_free_urb;

	usb_set_intfdata(intf, usbio);

	return 0;

err_free_urb:
	usb_free_urb(usbio->irq);

err_free_buffers:
	usbio_free_buffers(udev, usbio);

err_free_mem:
	if(usbio->mouse->input)
		input_free_device(usbio->mouse->input);
	if(usbio->keypad->input)
		input_free_device(usbio->keypad->input);
	if(usbio->sensor->input)
		input_free_device(usbio->sensor->input);

	return err;
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_disconnect  (struct usb_interface *intf)
{
	struct usbio *usbio = usb_get_intfdata(intf);

	if (!usbio)	return;

	dev_dbg(&intf->dev, "%s - ODROID USBIO Sensor Board is initialized, cleaning up\n", __func__);

	usb_set_intfdata(intf, NULL);

	usbio_sensor_disable(usbio);
	destroy_workqueue(usbio->sensor->wq);
	sysfs_remove_group(&usbio->sensor->input->dev.kobj,
				&usbio_sensor_sysfs_attrs_group);

	/* this will stop IO via close */
	if (usbio->mouse->input)
		input_unregister_device(usbio->mouse->input);
	if (usbio->keypad->input)
		input_unregister_device(usbio->keypad->input);
	if (usbio->sensor->input)
		input_unregister_device(usbio->sensor->input);

	usb_free_urb(usbio->irq);

	usbio_free_buffers(interface_to_usbdev(intf), usbio);
}

//[*]-------------------------------------------------------------------------[*]
MODULE_DEVICE_TABLE(usb, usbio_devices);

//[*]-------------------------------------------------------------------------[*]
static struct usb_driver usbio_driver = {
	.name		= "usbio",
	.probe		= usbio_probe,
	.disconnect	= usbio_disconnect,
	.suspend	= usbio_suspend,
	.resume		= usbio_resume,
	.reset_resume	= usbio_reset_resume,
	.id_table	= usbio_devices,
	.supports_autosuspend = 1,
};

//[*]-------------------------------------------------------------------------[*]
module_usb_driver(usbio_driver);

//[*]-------------------------------------------------------------------------[*]
MODULE_AUTHOR("Hardkernel Co.,Ltd");
MODULE_DESCRIPTION("ODROID USBIO Sensor Board Driver");
MODULE_LICENSE("GPL");

MODULE_ALIAS("usbio");

//[*]-------------------------------------------------------------------------[*]
