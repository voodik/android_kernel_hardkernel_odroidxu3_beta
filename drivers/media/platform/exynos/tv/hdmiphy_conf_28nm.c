/*
 * Samsung HDMI Physical interface driver
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Author: Ayoung Sim <a.sim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <uapi/linux/v4l2-dv-timings.h>
#include "hdmi.h"

static const u8 hdmiphy_conf25_2[32] = {
	0x01, 0x52, 0x3F, 0x55, 0x40, 0x01, 0x00, 0xC8,
	0x82, 0xC8, 0xBD, 0xD8, 0x45, 0xA0, 0xAC, 0x80,
	0x06, 0x80, 0x01, 0x84, 0x05, 0x02, 0x24, 0x66,
	0x54, 0xF4, 0x24, 0x00, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf27[32] = {
	0x01, 0xD1, 0x22, 0x51, 0x40, 0x08, 0xFC, 0xE0,
	0x98, 0xE8, 0xCB, 0xD8, 0x45, 0xA0, 0xAC, 0x80,
	0x06, 0x80, 0x09, 0x84, 0x05, 0x02, 0x24, 0x66,
	0x54, 0xE4, 0x24, 0x00, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf27_027[32] = {
	0x01, 0xd1, 0x2d, 0x72, 0x40, 0x64, 0x12, 0x08,
	0x43, 0xe8, 0x0e, 0xd9, 0x45, 0xa0, 0xac, 0x80,
	0x06, 0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86,
	0x54, 0xe3, 0x24, 0x00, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf40[32] = {
	0x01, 0xD1, 0x21, 0x31, 0x40, 0x3C, 0x28, 0xC8,
	0x87, 0xE8, 0xC8, 0xD8, 0x45, 0xA0, 0xAC, 0x80,
	0x08, 0x80, 0x09, 0x84, 0x05, 0x02, 0x24, 0x66,
	0x54, 0x9A, 0x24, 0x00, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf65[32] = {
	0x01, 0xD1, 0x36, 0x34, 0x40, 0x0C, 0x04, 0xC8,
	0x82, 0xE8, 0x45, 0xD9, 0x45, 0xA0, 0xAC, 0x80,
	0x08, 0x80, 0x09, 0x84, 0x05, 0x02, 0x24, 0x66,
	0x54, 0xBD, 0x24, 0x01, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf71[32] = {
	0x01, 0xD1, 0x3B, 0x35, 0x40, 0x0C, 0x04, 0xC8,
	0x85, 0xE8, 0x63, 0xD9, 0x45, 0xA0, 0xAC, 0x80,
	0x08, 0x80, 0x09, 0x84, 0x05, 0x02, 0x24, 0x66,
	0x54, 0x57, 0x24, 0x00, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf74_175[32] = {
	0x01, 0xd1, 0x1f, 0x10, 0x40, 0x5b, 0xef, 0x08,
	0x81, 0xe8, 0xb9, 0xd8, 0x45, 0xa0, 0xac, 0x80,
	0x56, 0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86,
	0x54, 0xa6, 0x24, 0x01, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf74_25[32] = {
	0x01, 0xd1, 0x1f, 0x10, 0x40, 0x40, 0xf8, 0x08,
	0x81, 0xe8, 0xba, 0xd8, 0x45, 0xa0, 0xac, 0x80,
	0x56, 0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86,
	0x54, 0xa5, 0x24, 0x01, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf88_75[32] = {
	0x01, 0xD1, 0x25, 0x11, 0x40, 0x18, 0xFF, 0xC8,
	0x83, 0xE8, 0xDE, 0xD8, 0x45, 0xA0, 0xAC, 0x80,
	0x08, 0x80, 0x09, 0x84, 0x05, 0x02, 0x24, 0x66,
	0x54, 0x45, 0x24, 0x00, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf108[32] = {
	0x01, 0x51, 0x2D, 0x15, 0x40, 0x01, 0x00, 0xC8,
	0x82, 0xC8, 0x0E, 0xD9, 0x45, 0xA0, 0xAC, 0x80,
	0x08, 0x80, 0x09, 0x84, 0x05, 0x02, 0x24, 0x66,
	0x54, 0xC7, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf148_352[32] = {
	0x01, 0xd1, 0x1f, 0x00, 0x40, 0x5b, 0xef, 0x08,
	0x81, 0xe8, 0xb9, 0xd8, 0x45, 0xa0, 0xac, 0x80,
	0x66, 0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86,
	0x54, 0x4b, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80,
};

static const u8 hdmiphy_conf148_5[32] = {
	0x01, 0xd1, 0x1f, 0x00, 0x40, 0x40, 0xf8, 0x08,
	0x81, 0xe8, 0xba, 0xd8, 0x45, 0xa0, 0xac, 0x80,
	0x66, 0x80, 0x09, 0x84, 0x05, 0x22, 0x24, 0x86,
	0x54, 0x4b, 0x25, 0x03, 0x00, 0x00, 0x01, 0x80,
};

const struct hdmiphy_conf hdmiphy_conf[] = {
	{ V4L2_DV_BT_DMT_640X480P60, hdmiphy_conf25_2},
	{ V4L2_DV_BT_CEA_720X480P59_94, hdmiphy_conf27 },
	{ V4L2_DV_BT_CEA_720X576P50, hdmiphy_conf27 },
	{ V4L2_DV_BT_DMT_800X600P60, hdmiphy_conf40 },
	{ V4L2_DV_BT_DMT_1024X768P60, hdmiphy_conf65 },
	{ V4L2_DV_BT_CEA_1280X720P50, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1280X720P60, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_DMT_1280X800P60_RB, hdmiphy_conf71 },
	{ V4L2_DV_BT_DMT_1280X960P60, hdmiphy_conf108 },
	{ V4L2_DV_BT_DMT_1440X900P60, hdmiphy_conf88_75 },
	{ V4L2_DV_BT_DMT_1280X1024P60, hdmiphy_conf108 },
	{ V4L2_DV_BT_CEA_1920X1080I50, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080I60, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080P24, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080P25, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080P30, hdmiphy_conf74_175 },
	{ V4L2_DV_BT_CEA_1920X1080P50, hdmiphy_conf148_5 },
	{ V4L2_DV_BT_CEA_1920X1080P60, hdmiphy_conf148_5 },
	{ V4L2_DV_BT_CEA_1280X720P60_SB_HALF, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1280X720P60_TB, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1280X720P50_SB_HALF, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1280X720P50_TB, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080P24_FP, hdmiphy_conf148_5 },
	{ V4L2_DV_BT_CEA_1920X1080P24_SB_HALF, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080P24_TB, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080I60_SB_HALF, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080I50_SB_HALF, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080P60_SB_HALF, hdmiphy_conf148_5 },
	{ V4L2_DV_BT_CEA_1920X1080P60_TB, hdmiphy_conf148_5 },
	{ V4L2_DV_BT_CEA_1920X1080P30_SB_HALF, hdmiphy_conf74_25 },
	{ V4L2_DV_BT_CEA_1920X1080P30_TB, hdmiphy_conf74_25 },
};

const int hdmiphy_conf_cnt = ARRAY_SIZE(hdmiphy_conf);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#if defined(CONFIG_ODROID_EXYNOS5_HDMI_PHY_TUNE)
//-----------------------------------------------------------------------------
//
//  TMDS data amplitude control.
//  1LSB corresponds to 20 mVdiff amplitude level.
//  tx_amp_lvl : 0 = 760 mVdiff(Min), 31 = 1380 mVdiff(Max)
//
//  Hardkernel default hdmi_tx_amp_lvl = 31(1380 mVdiff);
//  Samsung default hdmi_tx_amp_lvl = 13(1020 mVdiff);
//
//-----------------------------------------------------------------------------
unsigned long   gTxAmpLevel = 31;   // Default setup

static int __init hdmi_tx_amp_lvl(char *line)
{
    if(kstrtoul(line, 5, &gTxAmpLevel) != 0)    gTxAmpLevel = 31;

    if(gTxAmpLevel > 31)    gTxAmpLevel = 31;

    return  0;
}
__setup("hdmi_tx_amp_lvl=", hdmi_tx_amp_lvl);

//-----------------------------------------------------------------------------
//
//  TMDS data amplitude fine control for each channel.
//  1LSB corresponds to 20 mVdiff amplitude level.
//  tx_lvl : 0 = 0 mVdiff(Min), 3 = 60 mVdiff(Max)
//
//  Hardkernel default
//      hdmi_tx_lvl_ch0 = 3, hdmi_tx_lvl_ch1 = 3, hdmi_tx_lvl_ch2 = 3,
//  Samsung default
//      hdmi_tx_lvl_ch0 = 1, hdmi_tx_lvl_ch1 = 0, hdmi_tx_lvl_ch2 = 2,
//
//-----------------------------------------------------------------------------
unsigned long   gTxLevelCh0 = 3;    // Default setup
unsigned long   gTxLevelCh1 = 3;    // Default setup
unsigned long   gTxLevelCh2 = 3;    // Default setup

static int __init hdmi_tx_lvl_ch0(char *line)
{
    if(kstrtoul(line, 5, &gTxLevelCh0) != 0)    gTxLevelCh0 = 3;

    if(gTxLevelCh0 > 3)     gTxLevelCh0 = 3;

    return  0;
}

static int __init hdmi_tx_lvl_ch1(char *line)
{
    if(kstrtoul(line, 5, &gTxLevelCh1) != 0)    gTxLevelCh1 = 3;

    if(gTxLevelCh1 > 3)     gTxLevelCh1 = 3;

    return  0;
}

static int __init hdmi_tx_lvl_ch2(char *line)
{
    if(kstrtoul(line, 5, &gTxLevelCh2) != 0)    gTxLevelCh2 = 3;

    if(gTxLevelCh2 > 3)     gTxLevelCh2 = 3;

    return  0;
}
__setup("hdmi_tx_lvl_ch0=", hdmi_tx_lvl_ch0);
__setup("hdmi_tx_lvl_ch1=", hdmi_tx_lvl_ch1);
__setup("hdmi_tx_lvl_ch2=", hdmi_tx_lvl_ch2);

//-----------------------------------------------------------------------------
//
//  TMDS data pre-emphasis level control.
//  1LSB corresponds to -0.45dB emphasis level except for 1
//  tx_emp_lvl : 0 = 0 db(Min), 1 = -0.25 db, 2 = 0.7 db, 15 = -7.45 db(Max)
//
//  Hardkernel default hdmi_tx_emp_lvl = 6 (-2.50 db);
//  Samsung default hdmi_tx_emp_lvl = 6 (-2.50 db);
//
//-----------------------------------------------------------------------------
unsigned long   gTxEmpLevel = 6;    // Default setup

static int __init hdmi_tx_emp_lvl(char *line)
{
    if(kstrtoul(line, 5, &gTxEmpLevel) != 0)    gTxEmpLevel = 6;

    if(gTxEmpLevel > 15)    gTxEmpLevel = 6;

    return  0;
}
__setup("hdmi_tx_emp_lvl=", hdmi_tx_emp_lvl);

//-----------------------------------------------------------------------------
//
//  TMDS clock amplitude control.
//  1LSB corresponds to 20 mVdiff amplitude level.
//  clk_amp_lvl : 0 = 790 mVdiff(Min), 31 = 1410 mVdiff(Max)
//
//  Hardkernel default hdmi_clk_amp_lvl = 31 (1410 mVdiff)
//  Samsung default hdmi_clk_amp_lvl = 16 (1110 mVdiff)
//
//-----------------------------------------------------------------------------
unsigned long   gClkAmpLevel = 31;  // Default setup

static int __init hdmi_clk_amp_lvl(char *line)
{
    if(kstrtoul(line, 5, &gClkAmpLevel) != 0)   gClkAmpLevel = 31;

    if(gClkAmpLevel > 31)   gClkAmpLevel = 31;

    return  0;
}
__setup("hdmi_clk_amp_lvl=", hdmi_clk_amp_lvl);

//-----------------------------------------------------------------------------
//
//  TMDS data source termination resistor control.
//  tx_res :
//      0 = Source Termination OFF(Min), 1 = 200 ohm, 2 = 300 ohm, 3 = 120 ohm(Max)
//
//  Hardkernrel default hdmi_tx_res = 0 (Source Termination OFF)
//  Samsung default hdmi_tx_res = 0 (Source Termination OFF)
//
//-----------------------------------------------------------------------------
unsigned long   gTxRes = 0; // Default setup

static int __init hdmi_tx_res(char *line)
{
    if(kstrtoul(line, 5, &gTxRes) != 0) gTxRes = 0;

    if(gTxRes > 3)  gTxRes = 0;

    return  0;
}
__setup("hdmi_tx_res=", hdmi_tx_res);

//-----------------------------------------------------------------------------
#if defined(CONFIG_ODROID_EXYNOS5_HDMI_PHY_TUNE_DEBUG)
void hdmi_phy_tune_info(unsigned char *buffer)
{
    unsigned char  value;

    printk("========================================\n");
    printk("         HDMI PHY TUNE INFO\n");
    printk("========================================\n");
    value = buffer[16] & 0x0F;  value <<= 1;
    value |= (buffer[15] & 0x80) ? 0x01 : 0x00;

    printk("TX_AMP_LVL[%d] (760 mVdiff ~ 1380 mVdiff) = %d mVdiff\n",
        value, 760 + (unsigned short)value * 20);

    value = (buffer[4] & 0xC0) >> 6;
    printk("TX_LVL_CH0[%d] (0 mVdiff ~ 60 mVdiff) = %d mVdiff\n",
        value, value * 20);

    value = buffer[19] & 0x03;
    printk("TX_LVL_CH1[%d] (0 mVdiff ~ 60 mVdiff) = %d mVdiff\n",
        value, value * 20);

    value = buffer[23] & 0x03;
    printk("TX_LVL_CH2[%d] (0 mVdiff ~ 60 mVdiff) = %d mVdiff\n",
        value, value * 20);

    value = (buffer[16] & 0xF0) >> 4;
    printk("TX_EMP_LVL[%d] (0 db ~ -7.45 db) = ", value);

    if(value == 1)  printk("-0.25 db\n");
    else    {
        if(value)   printk("-%d.%02d db\n",
            (25 + 45 * (unsigned short)(value -1)) / 100,
            (25 + 45 * (unsigned short)(value -1)) % 100);
        else        printk("0 db\n");
    }

    value = (buffer[23] & 0xF8) >> 3;
    printk("TX_CLK_LVL[%d] (790 mVdiff ~ 1410 mVdiff) = %d mVdiff\n",
        value, 790 + (unsigned short)value * 20);

    value = (buffer[15] & 0x30) >> 4;
    printk("TX_RES[%d] = ", value);
    switch(value)   {
        case    0:
        default :   printk("Source Termination OFF\n");
            break;
        case    1:  printk("200 ohm\n");
            break;
        case    2:  printk("300 ohm\n");
            break;
        case    3:  printk("120 ohm\n");
            break;
    }
    printk("========================================\n");
}
#endif  // #if defined(CONFIG_ODROID_EXYNOS5_HDMI_PHY_TUNE_DEBUG)

//-----------------------------------------------------------------------------
void hdmi_phy_tune(unsigned char *buffer)
{
    // TxAmpLevel Control
    buffer[16] &= (~0x0F);    buffer[15] &= (~0x80);

    buffer[16] |= (gTxAmpLevel >> 1) & 0x0F;
    buffer[15] |= (gTxAmpLevel & 0x01) ? 0x80 : 0x00;

    // TxLevel Control 0
    buffer[4] &= (~0xC0);
    buffer[4] |= (gTxLevelCh0 << 6) & 0xC0;

    // TxLevel Control 1
    buffer[19] &= (~0x03);
    buffer[19] |= (gTxLevelCh1 & 0x03);

    // TxLevel Control 2
    buffer[23] &= (~0x03);
    buffer[23] |= (gTxLevelCh2 & 0x03);

    // TxEmpLevel Control
    buffer[16] &= (~0xF0);
    buffer[16] |= (gTxEmpLevel << 4) & 0xF0;

    //ClkAmpLevel Control
    buffer[23] &= (~0xF8);
    buffer[23] |= (gClkAmpLevel << 3) & 0xF8;

    // TxRes Control
    buffer[15] &= (~0x30);
    buffer[15] |= (gTxRes << 4) & 0x30;
}
#endif  // #if defined(CONFIG_ODROID_EXYNOS5_HDMI_PHY_TUNE)
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
