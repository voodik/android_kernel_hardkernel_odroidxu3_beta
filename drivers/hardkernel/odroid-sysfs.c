//[*]--------------------------------------------------------------------------------------------------[*]
//
//
// 
//  ODROID Board : ODROID sysfs driver (charles.park)
//  2012.01.17
// 
//
//[*]--------------------------------------------------------------------------------------------------[*]
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-pmu.h>
#include <plat/gpio-cfg.h>

//[*]--------------------------------------------------------------------------------------------------[*]
#define	DEBUG_PM_MSG

//[*]--------------------------------------------------------------------------------------------------[*]
// Sleep disable flage
//[*]--------------------------------------------------------------------------------------------------[*]
#define	SLEEP_DISABLE_FLAG

#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		#include <linux/wakelock.h>
		static struct wake_lock 	sleep_wake_lock;
	#endif
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
#define EXYNOS_BOOT_SDMMC		0x4
#define STATUS_TIMER_PEROID     1   // 1 sec

struct hrtimer      odroid_sysfs_timer;	    // odroid sysfs timer
struct input_dev    *vt_input;			    // virtual input driver

int Keycode[1] = {  KEY_POWER,  };
int KeyReleaseTime = 0;

//[*]--------------------------------------------------------------------------------------------------[*]
//
// Frame Buffer Size & Timming Bootargs parsing
//
//[*]--------------------------------------------------------------------------------------------------[*]
unsigned long   FbLeft = 0, FbRight = 0, FbUpper = 0;
unsigned long   FbLower = 0, FbHsync = 0, FbVsync = 0;
unsigned long   FbXres = 0, FbYres = 0;

EXPORT_SYMBOL(FbLeft);  EXPORT_SYMBOL(FbRight); EXPORT_SYMBOL(FbUpper);
EXPORT_SYMBOL(FbLower); EXPORT_SYMBOL(FbHsync); EXPORT_SYMBOL(FbVsync);
EXPORT_SYMBOL(FbXres);  EXPORT_SYMBOL(FbYres);

//[*]--------------------------------------------------------------------------------------------------[*]
static int __init framebuffer_xres(char *line)
{
    if(kstrtoul(line, 10, &FbXres) != 0)    FbXres = 0;    

    return  0;
}
__setup("fb_x_res=", framebuffer_xres);

static int __init framebuffer_yres(char *line)
{
    if(kstrtoul(line, 10, &FbYres) != 0)    FbYres = 0;    

    return  0;
}
__setup("fb_y_res=", framebuffer_yres);

static int __init timming_left(char *line)
{
    if(kstrtoul(line, 10, &FbLeft) != 0)    FbLeft = 0;    

    return  0;
}
__setup("left=", timming_left);

static int __init timming_right(char *line)
{
    if(kstrtoul(line, 10, &FbRight) != 0)   FbRight = 0;    

    return  0;
}
__setup("right=", timming_right);

static int __init timming_upper(char *line)
{
    if(kstrtoul(line, 10, &FbUpper) != 0)   FbUpper = 0;    

    return  0;
}
__setup("upper=", timming_upper);

static int __init timming_lower(char *line)
{
    if(kstrtoul(line, 10, &FbLower) != 0)   FbLower = 0;    

    return  0;
}
__setup("lower=", timming_lower);

static int __init timming_hsync(char *line)
{
    if(kstrtoul(line, 10, &FbHsync) != 0)   FbHsync = 0;    

    return  0;
}
__setup("hsync=", timming_hsync);

static int __init timming_vsync(char *line)
{
    if(kstrtoul(line, 10, &FbVsync) != 0)   FbVsync = 0;    

    return  0;
}
__setup("vsync=", timming_vsync);

//[*]--------------------------------------------------------------------------------------------------[*]
//
// HDMI PHY Bootargs parsing
//
//[*]--------------------------------------------------------------------------------------------------[*]
unsigned char   HdmiBootArgs[10] = { 0 };
EXPORT_SYMBOL(HdmiBootArgs);

// Bootargs parsing
static int __init hdmi_resolution_setup(char *line)
{
    memset(HdmiBootArgs, 0x00, sizeof(HdmiBootArgs));
    sprintf(HdmiBootArgs, "%s", line);
    return  0;
}
__setup("hdmi_phy_res=", hdmi_resolution_setup);

//[*]--------------------------------------------------------------------------------------------------[*]
//
// HDMI OUTPUT Mode(HDMI/DVI) Bootargs parsing
//
//[*]--------------------------------------------------------------------------------------------------[*]
unsigned char   VOutArgs[5] = { 0 };
EXPORT_SYMBOL(VOutArgs);

static int __init vout_mode_setup(char *line)
{
    memset(VOutArgs, 0x00, sizeof(VOutArgs));
    sprintf(VOutArgs, "%s", line);
    return  0;
}
__setup("v_out=", vout_mode_setup);

//[*]--------------------------------------------------------------------------------------------------[*]
//
//   sysfs function prototype define
//
//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_resolution         (struct device *dev, struct device_attribute *attr, char *buf);

static	ssize_t show_vout_mode			(struct device *dev, struct device_attribute *attr, char *buf);

static 	ssize_t set_poweroff_trigger    (struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static	ssize_t show_boot_mode          (struct device *dev, struct device_attribute *attr, char *buf);

static	ssize_t show_inform	            (struct device *dev, struct device_attribute *attr, char *buf);
static 	ssize_t set_inform	            (struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static	DEVICE_ATTR(hdmi_resolution,	S_IRWXUGO, show_resolution, NULL);
static	DEVICE_ATTR(vout_mode,			S_IRWXUGO, show_vout_mode, NULL);
static	DEVICE_ATTR(poweroff_trigger,	S_IRWXUGO, NULL, set_poweroff_trigger);
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static	DEVICE_ATTR(boot_mode,			S_IRWXUGO, show_boot_mode, NULL);
//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
#define INFORM_BASE     0x0800
#define INFORM_OFFSET   0x0004
#define INFORM_REG_CNT  8

static  DEVICE_ATTR(inform0, S_IRWXUGO, show_inform, set_inform);
static  DEVICE_ATTR(inform2, S_IRWXUGO, show_inform, set_inform);
static  DEVICE_ATTR(inform5, S_IRWXUGO, show_inform, set_inform);
static  DEVICE_ATTR(inform6, S_IRWXUGO, show_inform, set_inform);
static  DEVICE_ATTR(inform7, S_IRWXUGO, show_inform, set_inform);
//[*]----------------------- ---------------------------------------------------------------------------[*]
static struct attribute *odroid_sysfs_entries[] = {
	&dev_attr_hdmi_resolution.attr,
	&dev_attr_vout_mode.attr,
	&dev_attr_poweroff_trigger.attr,
	&dev_attr_boot_mode.attr,

    &dev_attr_inform0.attr,     // value clear xreset signal (used bootloader to kernel)
    &dev_attr_inform2.attr,     // value clear xreset signal (used bootloader to kernel)
    &dev_attr_inform5.attr,     // value clear power reset signal (used kernel to bootloader)
    &dev_attr_inform6.attr,     // value clear power reset signal (used kernel to bootloader)
    &dev_attr_inform7.attr,     // value clear power reset signal (used kernel to bootloader)
	NULL
};

static struct attribute_group odroid_sysfs_attr_group = {
	.name   = NULL,
	.attrs  = odroid_sysfs_entries,
};

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t show_inform		(struct device *dev, struct device_attribute *attr, char *buf)
{
	int	    i;
	char    name[7];

	for (i = 0; i < INFORM_REG_CNT; i++) {
	    
	    memset(name, 0x00, sizeof(name));	    sprintf(name, "inform%d", i);
	    
	    if(!strncmp(attr->attr.name, name, sizeof(name)))    {
	        return  sprintf(buf, "0x%08X\n", readl(EXYNOS_PMUREG(INFORM_BASE + i * INFORM_OFFSET)));
	    }
	}
	
	return	sprintf(buf, "ERROR! : Not found %s reg!\n", attr->attr.name);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_inform		(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long	val, i;
	char            name[7];

    if(buf[0] == '0' && ((buf[1] == 'X') || (buf[1] == 'x')))   {
        if(kstrtoul(buf, 16, &val) != 0)   val = 0;    
    }
    else    {
        if(kstrtoul(buf, 10, &val) != 0)   val = 0;    
    }

	for (i = 0; i < INFORM_REG_CNT; i++) {
	    
	    memset(name, 0x00, sizeof(name));	    sprintf(name, "inform%ld", i);
	    
	    if(!strncmp(attr->attr.name, name, sizeof(name)))    {
	        writel(val, EXYNOS_PMUREG(INFORM_BASE + i * INFORM_OFFSET)); 
		    return count;
	    }
	}

	printk("ERROR! : Not found %s reg!\n", attr->attr.name);
	return  count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_resolution (struct device *dev, struct device_attribute *attr, char *buf)
{
    return  (HdmiBootArgs[0] != 0) ? sprintf(buf, "%s\n", HdmiBootArgs) : sprintf(buf, "%s\n", "720p60hz");
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_vout_mode (struct device *dev, struct device_attribute *attr, char *buf)
{
    return  (VOutArgs[0] != 0) ? sprintf(buf, "%s\n", VOutArgs) : sprintf(buf, "%s\n", "hdmi");
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	ssize_t show_boot_mode (struct device *dev, struct device_attribute *attr, char *buf)
{
	return	sprintf(buf, "%d\n", (EXYNOS_BOOT_SDMMC == readl(EXYNOS5422_OM_STAT)) ? 1 : 0);
}

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static 	ssize_t set_poweroff_trigger    (struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned int	val;

    if(!(sscanf(buf, "%d\n", &val))) 	return	-EINVAL;

    //press power off button
    if((val != 0) && (val < 5))     {
        if(!KeyReleaseTime) {
            KeyReleaseTime = val;   input_report_key(vt_input, KEY_POWER, 1);
            hrtimer_start(&odroid_sysfs_timer, ktime_set(KeyReleaseTime, 0), HRTIMER_MODE_REL);
    		input_sync(vt_input);
        }
    }
    
    return 	count;
}

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
static int	odroid_sysfs_resume(struct platform_device *dev)
{
	#if	defined(DEBUG_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif

    return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	odroid_sysfs_suspend(struct platform_device *dev, pm_message_t state)
{
	#if	defined(DEBUG_PM_MSG)
		printk("%s\n", __FUNCTION__);
	#endif
	
    return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static enum hrtimer_restart odroid_sysfs_timer_function(struct hrtimer *timer)
{
    KeyReleaseTime = 0;
    input_report_key(vt_input, KEY_POWER, 0);	input_sync(vt_input);
    
	return HRTIMER_NORESTART;
}
//[*]--------------------------------------------------------------------------------------------------[*]
static	int		odroid_sysfs_probe		(struct platform_device *pdev)	
{
#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		wake_lock(&sleep_wake_lock);
	#endif
#endif
    //------------------------------------------------------------------------
    //
    // Virtual Key Init(Power Off Key)
    //
    //------------------------------------------------------------------------
	printk("--------------------------------------------------------\n");
	if((vt_input = input_allocate_device()))   {
        vt_input->name 		    = "vt-input";   vt_input->phys 	        = "vt-input/input0";       
        vt_input->id.bustype 	= BUS_HOST;     vt_input->id.vendor 	= 0x16B4;             
        vt_input->id.product 	= 0x0701;       vt_input->id.version 	= 0x0001;             
        vt_input->keycode 		= Keycode;   
    
    	set_bit(EV_KEY, vt_input->evbit);       set_bit(KEY_POWER & KEY_MAX, vt_input->keybit);
    
    	if(input_register_device(vt_input))	
    		printk("%s input register device fail!!\n", "Virtual-Key");
    	else    
    		printk("%s input driver registered!!\n", "Virtual-Key");
	}
	else
		printk("%s input register device fail!!\n", "Virtual-Key");

	printk("--------------------------------------------------------\n");

	hrtimer_init(&odroid_sysfs_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	odroid_sysfs_timer.function = odroid_sysfs_timer_function;

	return	sysfs_create_group(&pdev->dev.kobj, &odroid_sysfs_attr_group);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	int		odroid_sysfs_remove		(struct platform_device *pdev)	
{
#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		wake_unlock(&sleep_wake_lock);
	#endif
#endif

    sysfs_remove_group(&pdev->dev.kobj, &odroid_sysfs_attr_group);
    
    return	0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
#if defined(CONFIG_OF)
static const struct of_device_id odroid_sysfs_dt[] = {
	{ .compatible = "odroid-sysfs" },
	{ },
};
MODULE_DEVICE_TABLE(of, odroid_sysfs_dt);
#endif

static struct platform_driver odroid_sysfs_driver = {
	.driver = {
		.name = "odroid-sysfs",
		.owner = THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table = of_match_ptr(odroid_sysfs_dt),
#endif
	},
	.probe 		= odroid_sysfs_probe,
	.remove 	= odroid_sysfs_remove,
	.suspend	= odroid_sysfs_suspend,
	.resume		= odroid_sysfs_resume,
};

//[*]--------------------------------------------------------------------------------------------------[*]
static int __init odroid_sysfs_init(void)
{	
#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
		printk("--------------------------------------------------------\n");
		printk("%s(%d) : Sleep Disable Flag SET!!(Wake_lock_init)\n", __FUNCTION__, __LINE__);
		printk("--------------------------------------------------------\n");

	    wake_lock_init(&sleep_wake_lock, WAKE_LOCK_SUSPEND, "sleep_wake_lock");
	#endif
#else
	printk("--------------------------------------------------------\n");
	printk("%s(%d) : Sleep Enable !! \n", __FUNCTION__, __LINE__);
	printk("--------------------------------------------------------\n");
#endif

    return platform_driver_register(&odroid_sysfs_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void __exit odroid_sysfs_exit(void)
{
#if defined(SLEEP_DISABLE_FLAG)
	#ifdef CONFIG_HAS_WAKELOCK
	    wake_lock_destroy(&sleep_wake_lock);
	#endif
#endif
    platform_driver_unregister(&odroid_sysfs_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
module_init(odroid_sysfs_init);
module_exit(odroid_sysfs_exit);

//[*]--------------------------------------------------------------------------------------------------[*]
MODULE_DESCRIPTION("SYSFS driver for odroid-Dev board");
MODULE_AUTHOR("Hard-Kernel");
MODULE_LICENSE("GPL");

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
