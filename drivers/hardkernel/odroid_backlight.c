//[*]--------------------------------------------------------------------------------------------------[*]
//
//  ODROID Board : ODROID Backlight driver
//
//[*]--------------------------------------------------------------------------------------------------[*]
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/pwm.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/backlight.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>

//[*]--------------------------------------------------------------------------------------------------[*]
struct odroid_backlight {
	struct pwm_device 		*pwm;

    struct backlight_device *bd;

    // device tree platform data
	int pwm_id;
	int pwm_period_ns;
	int pwm_max;
	int pwm_default;
};

//[*]------------------------------------------------------------------------------------------------------------------
static int odroid_get_brightness(struct backlight_device *bd);
static int odroid_set_brightness(struct backlight_device *bd);

static const struct backlight_ops odroid_backlight_ops = {
	.get_brightness = odroid_get_brightness,
	.update_status = odroid_set_brightness,
};

//[*]------------------------------------------------------------------------------------------------------------------
static int odroid_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int odroid_set_brightness(struct backlight_device *bd)
{
    struct odroid_backlight *backlight = bd->dev.platform_data;

    int brightness = backlight->pwm_max - bd->props.brightness;

	pwm_config(backlight->pwm, backlight->pwm_period_ns * brightness / backlight->pwm_max, backlight->pwm_period_ns);
	pwm_enable(backlight->pwm);

	return 1;
}
//[*]--------------------------------------------------------------------------------------------------[*]
static int	odroid_backlight_resume(struct platform_device *dev)
{
    return  0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int	odroid_backlight_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static int odroid_backlight_parse_dt(struct platform_device *pdev, struct odroid_backlight *backlight)
{
	struct device 	*dev = &pdev->dev;
	struct device_node *np = dev->of_node;
    unsigned int    rdata;

	dev->platform_data = backlight;

	if(of_property_read_u32(np, "pwm_id", &rdata))	        return -1;
	backlight->pwm_id = rdata;

	if(of_property_read_u32(np, "pwm_period_ns", &rdata))	return -1;
	backlight->pwm_period_ns = rdata;

	if(of_property_read_u32(np, "pwm_max", &rdata))	        return -1;
	backlight->pwm_max = rdata;

	if(of_property_read_u32(np, "pwm_default", &rdata))	    return -1;
	backlight->pwm_default = rdata;
	
	return 0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	int		odroid_backlight_probe		(struct platform_device *pdev)	
{
	struct odroid_backlight *backlight;
	int ret = 0;

	backlight = devm_kzalloc(&pdev->dev, sizeof(*backlight), GFP_KERNEL);
	if (!backlight) {
		dev_err(&pdev->dev, "no memory for state\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	if (!pdev->dev.of_node) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "failed to dev.of_node!\n");
		goto err_alloc;
	}   
    
    if(odroid_backlight_parse_dt(pdev, backlight))  {
		ret = -ENODEV;
		dev_err(&pdev->dev, "failed to odroid_backlight_parse_dt!\n");
        goto err_alloc;
    }

	backlight->pwm = pwm_request(backlight->pwm_id, "pwm-backlight");
	
	if (IS_ERR(backlight->pwm)) {
		ret = -ENODEV;
		dev_err(&pdev->dev, "unable to request legacy PWM!\n");
		goto err_alloc;
	}

	pwm_config(backlight->pwm, backlight->pwm_period_ns * backlight->pwm_default / backlight->pwm_max, backlight->pwm_period_ns);
	pwm_enable(backlight->pwm);

	backlight->bd = backlight_device_register("pwm-backlight.0", NULL,
		NULL, &odroid_backlight_ops, NULL);

	if (IS_ERR(backlight->bd))  {
		dev_err(&pdev->dev, "failed to register backlight device!\n");
		ret = -ENODEV;
        goto err_alloc;
	}

	backlight->bd->dev.platform_data = backlight;
	backlight->bd->props.max_brightness = backlight->pwm_max;
	backlight->bd->props.brightness = backlight->pwm_default;

	return 0;
err_alloc:
	return ret;
}

//[*]--------------------------------------------------------------------------------------------------[*]
static	int		odroid_backlight_remove		(struct platform_device *pdev)	
{
    return	0;
}

//[*]--------------------------------------------------------------------------------------------------[*]
#if defined(CONFIG_OF)
static const struct of_device_id odroid_backlight_dt[] = {
	{ .compatible = "odroid-backlight" },
	{ },
};
MODULE_DEVICE_TABLE(of, odroid_backlight_dt);
#endif

//[*]--------------------------------------------------------------------------------------------------[*]
static struct platform_driver odroid_backlight_driver = {
	.driver = {
		.name = "odroid-backlight",
		.owner = THIS_MODULE,
#if defined(CONFIG_OF)
		.of_match_table = of_match_ptr(odroid_backlight_dt),
#endif
	},
	.probe 		= odroid_backlight_probe,
	.remove 	= odroid_backlight_remove,
	.suspend	= odroid_backlight_suspend,
	.resume		= odroid_backlight_resume,
};

//[*]--------------------------------------------------------------------------------------------------[*]
static int __init odroid_backlight_init(void)
{
    return platform_driver_register(&odroid_backlight_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
static void __exit odroid_backlight_exit(void)
{
    platform_driver_unregister(&odroid_backlight_driver);
}

//[*]--------------------------------------------------------------------------------------------------[*]
module_init(odroid_backlight_init);
module_exit(odroid_backlight_exit);

//[*]--------------------------------------------------------------------------------------------------[*]
MODULE_DESCRIPTION("ODROID-BACKLIGHT driver for odroid-Dev board");
MODULE_AUTHOR("Hard-Kernel");
MODULE_LICENSE("GPL");

//[*]--------------------------------------------------------------------------------------------------[*]
//[*]--------------------------------------------------------------------------------------------------[*]
