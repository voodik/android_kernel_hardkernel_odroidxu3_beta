/* drivers/video/decon_display/tc358764_mipi_lcd.c
 *
 * Samsung SoC MIPI LCD driver.
 *
 * Copyright (c) 2013 Samsung Electronics
 *
 * Haowei Li, <haowei.li@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/gpio.h>
#include <video/mipi_display.h>
#include <linux/platform_device.h>

struct decon_lcd * decon_get_lcd_info(void)
{
	return NULL;
}
EXPORT_SYMBOL(decon_get_lcd_info);

static int dummy_mipi_probe(struct mipi_dsim_device *dsim)
{
	return 1;
}

static int dummy_mipi_suspend(struct mipi_dsim_device *dsim)
{
	return 1;
}

static int dummy_mipi_resume(struct mipi_dsim_device *dsim)
{
	return 1;
}

struct mipi_dsim_lcd_driver tc358764_mipi_lcd_driver = {
	.probe		= dummy_mipi_probe,
	.suspend	= dummy_mipi_suspend,
	.resume		= dummy_mipi_resume,
};
