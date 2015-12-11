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
static int usbio_keypad_open (struct input_dev *input)
{
	return	0;
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_keypad_close (struct input_dev *input)
{
	return;
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_keypad_touch_process
		(struct usbio *usbio, unsigned short keypad_raw)
{
	unsigned short i, key_release;

	key_release = (keypad_raw ^ usbio->keypad->raw_old) & usbio->keypad->raw_old;

	for (i = 0; i < NUM_OF_KEYS; i++) {
		if (BUTTON[i].use && (keypad_raw & 0x0001)) {
			if (BUTTON[i].type == BUTTON_TYPE_TOUCH) {
				input_mt_slot(usbio->keypad->input, i);
				input_mt_report_slot_state(usbio->keypad->input,
							   MT_TOOL_FINGER, true);
				input_report_abs(usbio->keypad->input,
						 ABS_MT_POSITION_X,
						 BUTTON[i].x);
				input_report_abs(usbio->keypad->input,
						 ABS_MT_POSITION_Y,
						 BUTTON[i].y);
			}
		}

		if (BUTTON[i].use && (key_release & 0x0001)) {
			if (BUTTON[i].type == BUTTON_TYPE_TOUCH) {
				input_mt_slot(usbio->keypad->input, i);
				input_mt_report_slot_state(usbio->keypad->input,
							   MT_TOOL_FINGER, false);
			}
		}
		keypad_raw >>= 1;
		key_release >>= 1;
	}
}

//[*]-------------------------------------------------------------------------[*]
void usbio_keypad_process (struct usbio *usbio, unsigned short keypad_raw)
{
	unsigned short	key_press, key_release, i;

	usbio_keypad_touch_process(usbio, keypad_raw);

	key_press   = (keypad_raw ^ usbio->keypad->raw_old) & keypad_raw;
	key_release = (keypad_raw ^ usbio->keypad->raw_old) & usbio->keypad->raw_old;

	i = 0;
	while (key_press) {
		if ((key_press & 0x0001) && BUTTON[i].use) {
			if (BUTTON[i].type == BUTTON_TYPE_KEY) {
				input_report_key(usbio->keypad->input,
						 BUTTON[i].code,
						 KEY_PRESS);
			}
		}
		i++;	key_press >>= 1;
	}

	i = 0;
	while (key_release) {
		if ((key_release & 0x0001) && BUTTON[i].use) {
			if (BUTTON[i].type == BUTTON_TYPE_KEY) {
				input_report_key(usbio->keypad->input,
						 BUTTON[i].code,
						 KEY_RELEASE);
			}
		}
		i++;	key_release >>= 1;
	}
	usbio->keypad->raw_old = keypad_raw;
	input_sync(usbio->keypad->input);
}

//[*]-------------------------------------------------------------------------[*]
int usbio_keypad_init (struct usbio *usbio)
{
	int err, i;
	struct input_dev *input_dev = usbio->keypad->input;

	input_dev->name = KEYPAD_NAME;
	input_dev->phys = KEYPAD_PHYS;

	input_set_drvdata(input_dev, usbio);

	input_dev->open  = usbio_keypad_open;
	input_dev->close = usbio_keypad_close;

	input_dev->id.bustype = BUS_USB;

	/* KEY PAD Enable */
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X,  0, 1280, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,  0,  720, 0, 0);
	input_mt_init_slots(input_dev, NUM_OF_KEYS, 0);

	for (i = 0; i < NUM_OF_KEYS; i++)	{
		if (BUTTON[i].use && (BUTTON[i].type == BUTTON_TYPE_KEY))
			set_bit(BUTTON[i].code & KEY_MAX, input_dev->keybit);
	}
	if ((err = input_register_device(input_dev)))	{
		pr_err("%s - input_register_device failed, err: %d\n", __func__, err);
		return  err;
	}
	return	0;
}

//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
