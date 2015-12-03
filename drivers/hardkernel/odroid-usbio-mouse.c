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

#include "odroid-usbio.h"

//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
static int usbio_mouse_open (struct input_dev *input)
{
	return	0;
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_mouse_close (struct input_dev *input)
{
	return;
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_mouse_key_process (struct usbio *usbio, unsigned short keypad_raw)
{
	unsigned short	key_press, key_release, i;

	key_press   = (keypad_raw ^ usbio->mouse->raw_old) & keypad_raw;
	key_release = (keypad_raw ^ usbio->mouse->raw_old) & usbio->mouse->raw_old;

	i = 0;
	while(key_press) {
		if ((key_press & 0x0001) && BUTTON[i].use) {
			if (BUTTON[i].type == BUTTON_TYPE_MOUSE) {
				switch(BUTTON[i].code) {
					case KEY_LEFT:	case KEY_RIGHT:
					case KEY_UP:	case KEY_DOWN:
						break;
					default :
						input_report_key(usbio->mouse->input,
								 BUTTON[i].code,
								 KEY_PRESS);
						break;
				}
			}
		}
		i++;	key_press >>= 1;
	}

	i = 0;
	while(key_release) {
		if ((key_release & 0x0001) && BUTTON[i].use) {
			if (BUTTON[i].type == BUTTON_TYPE_MOUSE) {
				switch(BUTTON[i].code) {
					case KEY_LEFT:	case KEY_RIGHT:
					case KEY_UP:	case KEY_DOWN:
						break;
					default :
						input_report_key(usbio->mouse->input,
								 BUTTON[i].code,
								 KEY_RELEASE);
						break;
				}
			}
		}
		i++;	key_release >>= 1;
	}
	usbio->mouse->raw_old = keypad_raw;
}

//[*]-------------------------------------------------------------------------[*]
void usbio_mouse_process (struct usbio *usbio, unsigned short keypad_raw)
{
	int i;

	usbio_mouse_key_process (usbio, keypad_raw);

	for (i = 0; i < NUM_OF_KEYS; i++) {
		if (BUTTON[i].use && (keypad_raw & 0x0001)) {
			if (BUTTON[i].type == BUTTON_TYPE_MOUSE) {
				switch(BUTTON[i].code) {
					case KEY_UP: 
						input_report_rel(usbio->mouse->input,
								 REL_Y,
								 -MOUSE_WHELL_OFFSET);
					break;
					case KEY_DOWN:
						input_report_rel(usbio->mouse->input,
								 REL_Y,
								 MOUSE_WHELL_OFFSET);
					break;
					case KEY_LEFT:
						input_report_rel(usbio->mouse->input,
								 REL_X,
								 -MOUSE_WHELL_OFFSET);
					break;
					case KEY_RIGHT:
						input_report_rel(usbio->mouse->input,
								 REL_X,
								 MOUSE_WHELL_OFFSET);
					break;
					default :
					break;
				}
			}
		}
		keypad_raw >>= 1;
	}
	input_sync(usbio->mouse->input);
}

//[*]-------------------------------------------------------------------------[*]
int usbio_mouse_init (struct usbio *usbio)
{
	int err;
	struct input_dev *input_dev = usbio->mouse->input;

	input_dev->name = MOUSE_NAME;
	input_dev->phys = MOUSE_PHYS;

	input_set_drvdata(input_dev, usbio);

	input_dev->open	 = usbio_mouse_open;
	input_dev->close = usbio_mouse_close;

	input_dev->id.bustype = BUS_USB;

	/* KEY MOUSE Enable */
	set_bit(EV_REL, input_dev->evbit);
	set_bit(REL_X,  input_dev->relbit);
	set_bit(REL_Y,  input_dev->relbit);

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(BTN_RIGHT,  input_dev->keybit);
	set_bit(BTN_MIDDLE, input_dev->keybit);
	set_bit(BTN_LEFT,   input_dev->keybit);

	if ((err = input_register_device(input_dev)))	{
		pr_err("%s - input_register_device failed, err: %d\n", __func__, err);
		return  err;
	}

	return	0;
}

//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
