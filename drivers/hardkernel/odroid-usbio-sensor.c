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
const int ORIENTATION_TABLE[8][4] = {
	// swap xy, sign x, sign y, sign z
	{  0,  1,  1,  1 },  /*  X,  Y,  Z */
	{  1, -1,  1,  1 },  /* -Y,  X,  Z */
	{  0, -1, -1,  1 },  /* -X, -Y,  Z */
	{  1,  1, -1,  1 },  /*  Y, -X,  Z */

	{  0,  1, -1, -1 },  /*  X, -Y, -Z */
	{  1,  1,  1, -1 },  /*  Y,  X, -Z */
	{  0, -1,  1, -1 },  /* -X,  Y, -Z */
	{  1, -1, -1, -1 },  /* -Y, -X, -Z */
};

//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
void usbio_sensor_enable(struct usbio *usbio)
{
	if (!usbio->sensor->enabled) {
		usbio->sensor->enabled = true;
		pr_debug("%s: start timer\n", __func__);
		hrtimer_start(&usbio->sensor->timer, usbio->sensor->poll_delay,
			HRTIMER_MODE_REL);
	}
}

//[*]-------------------------------------------------------------------------[*]
void usbio_sensor_disable(struct usbio *usbio)
{
	if (usbio->sensor->enabled) {
		usbio->sensor->enabled = false;
		pr_debug("%s: stop timer\n", __func__);
		hrtimer_cancel(&usbio->sensor->timer);
		cancel_work_sync(&usbio->sensor->work);
	}
}

//[*]-------------------------------------------------------------------------[*]
static ssize_t usbio_sensor_poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct usbio *usbio = dev_get_drvdata(dev);

	return sprintf(buf, "%lld\n",
		ktime_to_ns(usbio->sensor->poll_delay));
}

//[*]-------------------------------------------------------------------------[*]
static ssize_t usbio_sensor_poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int err;
	int64_t new_delay;
	struct usbio *usbio = dev_get_drvdata(dev);

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	pr_debug("%s: new delay = %lldns, old delay = %lldns\n",
		__func__, new_delay, ktime_to_ns(usbio->sensor->poll_delay));

	if (new_delay < DELAY_LOWBOUND || new_delay > DELAY_UPBOUND)
		return -EINVAL;

	if (new_delay != ktime_to_ns(usbio->sensor->poll_delay))
		usbio->sensor->poll_delay = ns_to_ktime(new_delay);

	return size;
}

//[*]-------------------------------------------------------------------------[*]
static ssize_t usbio_sensor_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct usbio *usbio = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", usbio->sensor->enabled);
}

//[*]-------------------------------------------------------------------------[*]
static ssize_t usbio_sensor_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	bool new_value;
	struct usbio *usbio = dev_get_drvdata(dev);

	pr_debug("%s: enable %s\n", __func__, buf);

	if (sysfs_streq(buf, "1")) {
		new_value = true;
	} else if (sysfs_streq(buf, "0")) {
		new_value = false;
	} else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	pr_debug("%s: new_value = %d, old state = %d\n",
		__func__, new_value, usbio->sensor->enabled);

	if (new_value)
		usbio_sensor_enable(usbio);
	else
		usbio_sensor_disable(usbio);

	return size;
}

//[*]-------------------------------------------------------------------------[*]
static DEVICE_ATTR(poll_delay, S_IRWXU | S_IRWXG | S_IRWXO,
		usbio_sensor_poll_delay_show, usbio_sensor_poll_delay_store);

static DEVICE_ATTR(enable, S_IRWXU | S_IRWXG | S_IRWXO,
		usbio_sensor_enable_show, usbio_sensor_enable_store);

static struct attribute *usbio_sensor_sysfs_attrs[] = {
	&dev_attr_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

struct attribute_group usbio_sensor_sysfs_attrs_group = {
	.name  = NULL,
	.attrs = usbio_sensor_sysfs_attrs,
};

//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
static void usbio_sensor_work (struct work_struct *work)
{
	struct sensor *sensor =
	    container_of(work, struct sensor, work);

	input_report_abs(sensor->input, ABS_X, sensor->x);
	input_report_abs(sensor->input, ABS_Y, sensor->y);
	input_report_abs(sensor->input, ABS_Z, sensor->z);
	input_sync(sensor->input);
#if defined(DEBUG_SENSOR)
	pr_err("%s: X = %d, Y = %d, Z = %d\n", __func__, sensor->x,
							 sensor->y,
							 sensor->z);
#endif
}

//[*]-------------------------------------------------------------------------[*]
static enum hrtimer_restart usbio_sensor_timer_func (struct hrtimer *timer)
{
	struct sensor *sensor =
	    container_of(timer, struct sensor, timer);

	pr_debug("%s: start\n", __func__);
	queue_work(sensor->wq, &sensor->work);
	hrtimer_forward_now(&sensor->timer, sensor->poll_delay);
	return HRTIMER_RESTART;
}

//[*]-------------------------------------------------------------------------[*]
static int usbio_sensor_open (struct input_dev *input)
{
	struct usbio *usbio = input_get_drvdata(input);
	int r;

	usbio->irq->dev = interface_to_usbdev(usbio->interface);

	r = usb_autopm_get_interface(usbio->interface) ? -EIO : 0;
	if (r < 0)
		goto out;

	if (usb_submit_urb(usbio->irq, GFP_KERNEL))	{
		r = -EIO;
		goto out_put;
	}

	usbio->interface->needs_remote_wakeup = 1;
out_put:
	usb_autopm_put_interface(usbio->interface);
out:
	return r;
}

//[*]-------------------------------------------------------------------------[*]
static void usbio_sensor_close   (struct input_dev *input)
{
	struct usbio *usbio = input_get_drvdata(input);
	int r;

	usb_kill_urb(usbio->irq);

	r = usb_autopm_get_interface(usbio->interface);

	usbio->interface->needs_remote_wakeup = 0;
	if (!r)
		usb_autopm_put_interface(usbio->interface);
}

//[*]-------------------------------------------------------------------------[*]
void usbio_sensor_process (struct usbio *usbio, unsigned char *pkt)
{
	struct  usbio_raw	*usbio_raw = (struct usbio_raw *)pkt;

    	/* Data x,y swap check */
    	if(ORIENTATION_TABLE[Orientation][0])	{
		usbio->sensor->x = 
			usbio_raw->y * ORIENTATION_TABLE[Orientation][1];
		usbio->sensor->y = 
			usbio_raw->x * ORIENTATION_TABLE[Orientation][2];
		usbio->sensor->z = 
			usbio_raw->z * ORIENTATION_TABLE[Orientation][3];
    	}
    	else	{
		usbio->sensor->x = 
			usbio_raw->x * ORIENTATION_TABLE[Orientation][1];
		usbio->sensor->y = 
			usbio_raw->y * ORIENTATION_TABLE[Orientation][2];
		usbio->sensor->z = 
			usbio_raw->z * ORIENTATION_TABLE[Orientation][3];
    	}
}

//[*]-------------------------------------------------------------------------[*]
int usbio_sensor_init (struct usbio *usbio)
{
	int err;
	struct input_dev *input_dev = usbio->sensor->input;

	input_dev->name = usbio->name;
	input_dev->phys = usbio->phys;

	input_set_drvdata(input_dev, usbio);

	input_dev->open  = usbio_sensor_open;
	input_dev->close = usbio_sensor_close;

	input_dev->id.bustype = BUS_USB;

	/* ABS Enable for Sensor data */
	set_bit(EV_ABS, input_dev->evbit);

	input_set_abs_params(input_dev, ABS_X, USBIO_SENSOR_MIN_X, USBIO_SENSOR_MAX_X, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, USBIO_SENSOR_MIN_Y, USBIO_SENSOR_MAX_Y, 0, 0);
	input_set_abs_params(input_dev, ABS_Z, USBIO_SENSOR_MIN_Z, USBIO_SENSOR_MAX_Z, 0, 0);

	if ((err = input_register_device(input_dev)))	{
		pr_err("%s - input_register_device failed, err: %d\n", __func__, err);
		return  err;
	}

	hrtimer_init(&usbio->sensor->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	usbio->sensor->poll_delay = ns_to_ktime(DELAY_DEFAULT);
	usbio->sensor->timer.function = usbio_sensor_timer_func;

	usbio->sensor->wq = alloc_workqueue("usbio_sensor_wq", WQ_UNBOUND, 1);

	if (!usbio->sensor->wq) {
		pr_err("%s: could not create workqueue\n", __func__);
		err = -ENOMEM;
		return	err;
	}

	INIT_WORK(&usbio->sensor->work, usbio_sensor_work);

	err = sysfs_create_group(&usbio->sensor->input->dev.kobj,
		&usbio_sensor_sysfs_attrs_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		destroy_workqueue(usbio->sensor->wq);
		return	err;
	}

	return 0;
}

//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
