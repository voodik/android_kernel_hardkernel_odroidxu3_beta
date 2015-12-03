//[*]-------------------------------------------------------------------------[*]
//
// ODROID USBIO + Accelerometer Sensor Driver (USB HID Protocol)
// Hardkernel : 2015/11/09 Charles.park
//
//[*]-------------------------------------------------------------------------[*]
#if !defined(__ODROID_USBIO_H__)

#define	__ODROID_USBIO_H__
//[*]-------------------------------------------------------------------------[*]

/* #define	DEBUG_SENSOR */

//[*]-------------------------------------------------------------------------[*]
#define USB_VENDOR_ID_USBIO		0x04D8
#define USB_DEVICE_ID_USBIO_SENSOR	0x003F

static const struct usb_device_id usbio_devices[] = {
	{USB_DEVICE(USB_VENDOR_ID_USBIO, USB_DEVICE_ID_USBIO_SENSOR), 0},
	{}
};

//[*]-------------------------------------------------------------------------[*]
#define DELAY_LOWBOUND		(50 * NSEC_PER_MSEC)
#define DELAY_UPBOUND		(500 * NSEC_PER_MSEC)
#define DELAY_DEFAULT		(200 * NSEC_PER_MSEC)

#define	USBIO_HEADER_PROTOCOL	0xAA
#define	USBIO_TAIL_PROTOCOL	0xCC

#define	USBIO_SENSOR_MAX_X	0x01FF
#define	USBIO_SENSOR_MIN_X	-(USBIO_SENSOR_MAX_X)
#define	USBIO_SENSOR_MAX_Y	0x01FF
#define	USBIO_SENSOR_MIN_Y	-(USBIO_SENSOR_MAX_X)
#define	USBIO_SENSOR_MAX_Z	0x01FF
#define	USBIO_SENSOR_MIN_Z	-(USBIO_SENSOR_MAX_X)

//[*]-------------------------------------------------------------------------[*]
#define NUM_OF_KEYS		16
#define	KEY_PRESS		1
#define	KEY_RELEASE		0

#define	BUTTON_TYPE_KEY		'K'
#define	BUTTON_TYPE_TOUCH	'T'
#define	BUTTON_TYPE_MOUSE	'M'

struct button {
	unsigned long	use;	/* 0 = not use, 1 = use */
	unsigned long	type;	/* 0 = key(parse 'k'), 1 = virtual touch bt(parse 't') */
	unsigned long	code; 	/* use in keymode */
	unsigned long	x;	/* use in virtual touch bt mode */
	unsigned long	y;
};

extern struct button BUTTON[NUM_OF_KEYS];

//[*]-------------------------------------------------------------------------[*]
#define	SENSOR_NAME	"usbio-sensor"
#define	SENSOR_PHYS	"usbio-sensor/input0"

extern unsigned long Orientation;

struct sensor {
	/* Sensor Data */
	struct workqueue_struct *wq;
	struct work_struct 	work;
	struct hrtimer 		timer;
	struct input_dev	*input;

	signed short	x;
	signed short	y;
	signed short	z;
	bool		enabled;
	ktime_t 	poll_delay;
	bool		on_before_suspend;
};

//[*]-------------------------------------------------------------------------[*]
#define	KEYPAD_NAME	"usbio-keypad"
#define KEYPAD_PHYS	"usbio-keypad/input0"

struct keypad {
	/* Keypad Data */
	unsigned short	 raw_old;
	struct input_dev *input;
};

//[*]-------------------------------------------------------------------------[*]
#define	MOUSE_NAME	"usbio-mouse"
#define MOUSE_PHYS	"usbio-mouse/input0"
#define	MOUSE_WHELL_OFFSET	10

struct mouse {
	/* mouse Data */
	unsigned short	 raw_old;
	struct input_dev *input;
};

//[*]-------------------------------------------------------------------------[*]
struct usbio_raw {		/* Total 8 bytes */
	unsigned char	header;	/* frame header 0xAA*/
	signed short	x;	/* Acc X data */
	signed short	y;	/* Acc Y data */
	signed short	z;	/* Acc Z data */
	unsigned short	key;
	unsigned char	tail;	/* frame end 0xCC */
}	__attribute__ ((packed));

struct usbio {
	char	name[128], phys[64];

	/* for URB Data DMA */
	dma_addr_t	data_dma;
	unsigned char	*data;
	int		data_size;

	struct urb		*irq;
	struct usb_interface	*interface;
	
	struct sensor	*sensor;
	struct keypad	*keypad;
	struct mouse	*mouse;
};

/* usbio-keypad.c */
extern	void usbio_keypad_process (struct usbio *usbio, unsigned short keypad_raw);
extern	int usbio_keypad_init     (struct usbio *usbio);

/* usbio-mouse.c */
extern	void usbio_mouse_process (struct usbio *usbio, unsigned short keypad_raw);
extern	int  usbio_mouse_init    (struct usbio *usbio);

/* usbio-sensor.c */
extern	struct attribute_group usbio_sensor_sysfs_attrs_group;

extern	void usbio_sensor_enable  (struct usbio *usbio);
extern	void usbio_sensor_disable (struct usbio *usbio);
extern	void usbio_sensor_process (struct usbio *usbio, unsigned char *pkt);
extern	int  usbio_sensor_init    (struct usbio *usbio);

#endif //(__ODROID_USBIO_H__)
//[*]-------------------------------------------------------------------------[*]
//[*]-------------------------------------------------------------------------[*]
