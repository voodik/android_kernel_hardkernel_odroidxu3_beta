/*
 * Realtek RTL2832 DVB-T demodulator driver
 *
 * Copyright (C) 2012 Thomas Mair <thomas.mair86@gmail.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "rtl2832_priv.h"
#include "dvb_math.h"
#include <linux/bitops.h>

#define REG_MASK(b) (BIT(b + 1) - 1)

static const struct rtl2832_reg_entry registers[] = {
	[DVBT_SOFT_RST]		= {0x1, 0x1,   2, 2},
	[DVBT_IIC_REPEAT]	= {0x1, 0x1,   3, 3},
	[DVBT_TR_WAIT_MIN_8K]	= {0x1, 0x88, 11, 2},
	[DVBT_RSD_BER_FAIL_VAL]	= {0x1, 0x8f, 15, 0},
	[DVBT_EN_BK_TRK]	= {0x1, 0xa6,  7, 7},
	[DVBT_AD_EN_REG]	= {0x0, 0x8,   7, 7},
	[DVBT_AD_EN_REG1]	= {0x0, 0x8,   6, 6},
	[DVBT_EN_BBIN]		= {0x1, 0xb1,  0, 0},
	[DVBT_MGD_THD0]		= {0x1, 0x95,  7, 0},
	[DVBT_MGD_THD1]		= {0x1, 0x96,  7, 0},
	[DVBT_MGD_THD2]		= {0x1, 0x97,  7, 0},
	[DVBT_MGD_THD3]		= {0x1, 0x98,  7, 0},
	[DVBT_MGD_THD4]		= {0x1, 0x99,  7, 0},
	[DVBT_MGD_THD5]		= {0x1, 0x9a,  7, 0},
	[DVBT_MGD_THD6]		= {0x1, 0x9b,  7, 0},
	[DVBT_MGD_THD7]		= {0x1, 0x9c,  7, 0},
	[DVBT_EN_CACQ_NOTCH]	= {0x1, 0x61,  4, 4},
	[DVBT_AD_AV_REF]	= {0x0, 0x9,   6, 0},
	[DVBT_REG_PI]		= {0x0, 0xa,   2, 0},
	[DVBT_PIP_ON]		= {0x0, 0x21,  3, 3},
	[DVBT_SCALE1_B92]	= {0x2, 0x92,  7, 0},
	[DVBT_SCALE1_B93]	= {0x2, 0x93,  7, 0},
	[DVBT_SCALE1_BA7]	= {0x2, 0xa7,  7, 0},
	[DVBT_SCALE1_BA9]	= {0x2, 0xa9,  7, 0},
	[DVBT_SCALE1_BAA]	= {0x2, 0xaa,  7, 0},
	[DVBT_SCALE1_BAB]	= {0x2, 0xab,  7, 0},
	[DVBT_SCALE1_BAC]	= {0x2, 0xac,  7, 0},
	[DVBT_SCALE1_BB0]	= {0x2, 0xb0,  7, 0},
	[DVBT_SCALE1_BB1]	= {0x2, 0xb1,  7, 0},
	[DVBT_KB_P1]		= {0x1, 0x64,  3, 1},
	[DVBT_KB_P2]		= {0x1, 0x64,  6, 4},
	[DVBT_KB_P3]		= {0x1, 0x65,  2, 0},
	[DVBT_OPT_ADC_IQ]	= {0x0, 0x6,   5, 4},
	[DVBT_AD_AVI]		= {0x0, 0x9,   1, 0},
	[DVBT_AD_AVQ]		= {0x0, 0x9,   3, 2},
	[DVBT_K1_CR_STEP12]	= {0x2, 0xad,  9, 4},
	[DVBT_TRK_KS_P2]	= {0x1, 0x6f,  2, 0},
	[DVBT_TRK_KS_I2]	= {0x1, 0x70,  5, 3},
	[DVBT_TR_THD_SET2]	= {0x1, 0x72,  3, 0},
	[DVBT_TRK_KC_P2]	= {0x1, 0x73,  5, 3},
	[DVBT_TRK_KC_I2]	= {0x1, 0x75,  2, 0},
	[DVBT_CR_THD_SET2]	= {0x1, 0x76,  7, 6},
	[DVBT_PSET_IFFREQ]	= {0x1, 0x19, 21, 0},
	[DVBT_SPEC_INV]		= {0x1, 0x15,  0, 0},
	[DVBT_RSAMP_RATIO]	= {0x1, 0x9f, 27, 2},
	[DVBT_CFREQ_OFF_RATIO]	= {0x1, 0x9d, 23, 4},
	[DVBT_FSM_STAGE]	= {0x3, 0x51,  6, 3},
	[DVBT_RX_CONSTEL]	= {0x3, 0x3c,  3, 2},
	[DVBT_RX_HIER]		= {0x3, 0x3c,  6, 4},
	[DVBT_RX_C_RATE_LP]	= {0x3, 0x3d,  2, 0},
	[DVBT_RX_C_RATE_HP]	= {0x3, 0x3d,  5, 3},
	[DVBT_GI_IDX]		= {0x3, 0x51,  1, 0},
	[DVBT_FFT_MODE_IDX]	= {0x3, 0x51,  2, 2},
	[DVBT_RSD_BER_EST]	= {0x3, 0x4e, 15, 0},
	[DVBT_CE_EST_EVM]	= {0x4, 0xc,  15, 0},
	[DVBT_RF_AGC_VAL]	= {0x3, 0x5b, 13, 0},
	[DVBT_IF_AGC_VAL]	= {0x3, 0x59, 13, 0},
	[DVBT_DAGC_VAL]		= {0x3, 0x5,   7, 0},
	[DVBT_SFREQ_OFF]	= {0x3, 0x18, 13, 0},
	[DVBT_CFREQ_OFF]	= {0x3, 0x5f, 17, 0},
	[DVBT_POLAR_RF_AGC]	= {0x0, 0xe,   1, 1},
	[DVBT_POLAR_IF_AGC]	= {0x0, 0xe,   0, 0},
	[DVBT_AAGC_HOLD]	= {0x1, 0x4,   5, 5},
	[DVBT_EN_RF_AGC]	= {0x1, 0x4,   6, 6},
	[DVBT_EN_IF_AGC]	= {0x1, 0x4,   7, 7},
	[DVBT_IF_AGC_MIN]	= {0x1, 0x8,   7, 0},
	[DVBT_IF_AGC_MAX]	= {0x1, 0x9,   7, 0},
	[DVBT_RF_AGC_MIN]	= {0x1, 0xa,   7, 0},
	[DVBT_RF_AGC_MAX]	= {0x1, 0xb,   7, 0},
	[DVBT_IF_AGC_MAN]	= {0x1, 0xc,   6, 6},
	[DVBT_IF_AGC_MAN_VAL]	= {0x1, 0xc,  13, 0},
	[DVBT_RF_AGC_MAN]	= {0x1, 0xe,   6, 6},
	[DVBT_RF_AGC_MAN_VAL]	= {0x1, 0xe,  13, 0},
	[DVBT_DAGC_TRG_VAL]	= {0x1, 0x12,  7, 0},
	[DVBT_AGC_TARG_VAL_0]	= {0x1, 0x2,   0, 0},
	[DVBT_AGC_TARG_VAL_8_1]	= {0x1, 0x3,   7, 0},
	[DVBT_AAGC_LOOP_GAIN]	= {0x1, 0xc7,  5, 1},
	[DVBT_LOOP_GAIN2_3_0]	= {0x1, 0x4,   4, 1},
	[DVBT_LOOP_GAIN2_4]	= {0x1, 0x5,   7, 7},
	[DVBT_LOOP_GAIN3]	= {0x1, 0xc8,  4, 0},
	[DVBT_VTOP1]		= {0x1, 0x6,   5, 0},
	[DVBT_VTOP2]		= {0x1, 0xc9,  5, 0},
	[DVBT_VTOP3]		= {0x1, 0xca,  5, 0},
	[DVBT_KRF1]		= {0x1, 0xcb,  7, 0},
	[DVBT_KRF2]		= {0x1, 0x7,   7, 0},
	[DVBT_KRF3]		= {0x1, 0xcd,  7, 0},
	[DVBT_KRF4]		= {0x1, 0xce,  7, 0},
	[DVBT_EN_GI_PGA]	= {0x1, 0xe5,  0, 0},
	[DVBT_THD_LOCK_UP]	= {0x1, 0xd9,  8, 0},
	[DVBT_THD_LOCK_DW]	= {0x1, 0xdb,  8, 0},
	[DVBT_THD_UP1]		= {0x1, 0xdd,  7, 0},
	[DVBT_THD_DW1]		= {0x1, 0xde,  7, 0},
	[DVBT_INTER_CNT_LEN]	= {0x1, 0xd8,  3, 0},
	[DVBT_GI_PGA_STATE]	= {0x1, 0xe6,  3, 3},
	[DVBT_EN_AGC_PGA]	= {0x1, 0xd7,  0, 0},
	[DVBT_CKOUTPAR]		= {0x1, 0x7b,  5, 5},
	[DVBT_CKOUT_PWR]	= {0x1, 0x7b,  6, 6},
	[DVBT_SYNC_DUR]		= {0x1, 0x7b,  7, 7},
	[DVBT_ERR_DUR]		= {0x1, 0x7c,  0, 0},
	[DVBT_SYNC_LVL]		= {0x1, 0x7c,  1, 1},
	[DVBT_ERR_LVL]		= {0x1, 0x7c,  2, 2},
	[DVBT_VAL_LVL]		= {0x1, 0x7c,  3, 3},
	[DVBT_SERIAL]		= {0x1, 0x7c,  4, 4},
	[DVBT_SER_LSB]		= {0x1, 0x7c,  5, 5},
	[DVBT_CDIV_PH0]		= {0x1, 0x7d,  3, 0},
	[DVBT_CDIV_PH1]		= {0x1, 0x7d,  7, 4},
	[DVBT_MPEG_IO_OPT_2_2]	= {0x0, 0x6,   7, 7},
	[DVBT_MPEG_IO_OPT_1_0]	= {0x0, 0x7,   7, 6},
	[DVBT_CKOUTPAR_PIP]	= {0x0, 0xb7,  4, 4},
	[DVBT_CKOUT_PWR_PIP]	= {0x0, 0xb7,  3, 3},
	[DVBT_SYNC_LVL_PIP]	= {0x0, 0xb7,  2, 2},
	[DVBT_ERR_LVL_PIP]	= {0x0, 0xb7,  1, 1},
	[DVBT_VAL_LVL_PIP]	= {0x0, 0xb7,  0, 0},
	[DVBT_CKOUTPAR_PID]	= {0x0, 0xb9,  4, 4},
	[DVBT_CKOUT_PWR_PID]	= {0x0, 0xb9,  3, 3},
	[DVBT_SYNC_LVL_PID]	= {0x0, 0xb9,  2, 2},
	[DVBT_ERR_LVL_PID]	= {0x0, 0xb9,  1, 1},
	[DVBT_VAL_LVL_PID]	= {0x0, 0xb9,  0, 0},
	[DVBT_SM_PASS]		= {0x1, 0x93, 11, 0},
	[DVBT_AD7_SETTING]	= {0x0, 0x11, 15, 0},
	[DVBT_RSSI_R]		= {0x3, 0x1,   6, 0},
	[DVBT_ACI_DET_IND]	= {0x3, 0x12,  0, 0},
	[DVBT_REG_MON]		= {0x0, 0xd,   1, 0},
	[DVBT_REG_MONSEL]	= {0x0, 0xd,   2, 2},
	[DVBT_REG_GPE]		= {0x0, 0xd,   7, 7},
	[DVBT_REG_GPO]		= {0x0, 0x10,  0, 0},
	[DVBT_REG_4MSEL]	= {0x0, 0x13,  0, 0},
};

/* Our regmap is bypassing I2C adapter lock, thus we do it! */
int rtl2832_bulk_write(struct i2c_client *client, unsigned int reg,
		       const void *val, size_t val_count)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);
	int ret;

	i2c_lock_adapter(client->adapter);
	ret = regmap_bulk_write(dev->regmap, reg, val, val_count);
	i2c_unlock_adapter(client->adapter);
	return ret;
}

int rtl2832_update_bits(struct i2c_client *client, unsigned int reg,
			unsigned int mask, unsigned int val)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);
	int ret;

	i2c_lock_adapter(client->adapter);
	ret = regmap_update_bits(dev->regmap, reg, mask, val);
	i2c_unlock_adapter(client->adapter);
	return ret;
}

int rtl2832_bulk_read(struct i2c_client *client, unsigned int reg, void *val,
		      size_t val_count)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);
	int ret;

	i2c_lock_adapter(client->adapter);
	ret = regmap_bulk_read(dev->regmap, reg, val, val_count);
	i2c_unlock_adapter(client->adapter);
	return ret;
}

/* write multiple registers */
static int rtl2832_wr_regs(struct rtl2832_dev *dev, u8 reg, u8 page, u8 *val, int len)
{
	return rtl2832_bulk_write(dev->client, page << 8 | reg, val, len);
}

/* read multiple registers */
static int rtl2832_rd_regs(struct rtl2832_dev *dev, u8 reg, u8 page, u8 *val, int len)
{
	return rtl2832_bulk_read(dev->client, page << 8 | reg, val, len);
}

/* write single register */
static int rtl2832_wr_reg(struct rtl2832_dev *dev, u8 reg, u8 page, u8 val)
{
	return rtl2832_wr_regs(dev, reg, page, &val, 1);
}

/* read single register */
static int rtl2832_rd_reg(struct rtl2832_dev *dev, u8 reg, u8 page, u8 *val)
{
	return rtl2832_rd_regs(dev, reg, page, val, 1);
}

static int rtl2832_rd_demod_reg(struct rtl2832_dev *dev, int reg, u32 *val)
{
	struct i2c_client *client = dev->client;
	int ret;

	u8 reg_start_addr;
	u8 msb, lsb;
	u8 page;
	u8 reading[4];
	u32 reading_tmp;
	int i;

	u8 len;
	u32 mask;

	reg_start_addr = registers[reg].start_address;
	msb = registers[reg].msb;
	lsb = registers[reg].lsb;
	page = registers[reg].page;

	len = (msb >> 3) + 1;
	mask = REG_MASK(msb - lsb);

	ret = rtl2832_rd_regs(dev, reg_start_addr, page, &reading[0], len);
	if (ret)
		goto err;

	reading_tmp = 0;
	for (i = 0; i < len; i++)
		reading_tmp |= reading[i] << ((len - 1 - i) * 8);

	*val = (reading_tmp >> lsb) & mask;

	return ret;

err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;

}

static int rtl2832_wr_demod_reg(struct rtl2832_dev *dev, int reg, u32 val)
{
	struct i2c_client *client = dev->client;
	int ret, i;
	u8 len;
	u8 reg_start_addr;
	u8 msb, lsb;
	u8 page;
	u32 mask;


	u8 reading[4];
	u8 writing[4];
	u32 reading_tmp;
	u32 writing_tmp;


	reg_start_addr = registers[reg].start_address;
	msb = registers[reg].msb;
	lsb = registers[reg].lsb;
	page = registers[reg].page;

	len = (msb >> 3) + 1;
	mask = REG_MASK(msb - lsb);


	ret = rtl2832_rd_regs(dev, reg_start_addr, page, &reading[0], len);
	if (ret)
		goto err;

	reading_tmp = 0;
	for (i = 0; i < len; i++)
		reading_tmp |= reading[i] << ((len - 1 - i) * 8);

	writing_tmp = reading_tmp & ~(mask << lsb);
	writing_tmp |= ((val & mask) << lsb);


	for (i = 0; i < len; i++)
		writing[i] = (writing_tmp >> ((len - 1 - i) * 8)) & 0xff;

	ret = rtl2832_wr_regs(dev, reg_start_addr, page, &writing[0], len);
	if (ret)
		goto err;

	return ret;

err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;

}

static int rtl2832_i2c_gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;
	int ret;

	dev_dbg(&client->dev, "enable=%d\n", enable);

	/* gate already open or close */
	if (dev->i2c_gate_state == enable)
		return 0;

	ret = rtl2832_wr_demod_reg(dev, DVBT_IIC_REPEAT, (enable ? 0x1 : 0x0));
	if (ret)
		goto err;

	dev->i2c_gate_state = enable;

	return ret;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_set_if(struct dvb_frontend *fe, u32 if_freq)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;
	int ret;
	u64 pset_iffreq;
	u8 en_bbin = (if_freq == 0 ? 0x1 : 0x0);

	/*
	* PSET_IFFREQ = - floor((IfFreqHz % CrystalFreqHz) * pow(2, 22)
	*		/ CrystalFreqHz)
	*/

	pset_iffreq = if_freq % dev->pdata->clk;
	pset_iffreq *= 0x400000;
	pset_iffreq = div_u64(pset_iffreq, dev->pdata->clk);
	pset_iffreq = -pset_iffreq;
	pset_iffreq = pset_iffreq & 0x3fffff;
	dev_dbg(&client->dev, "if_frequency=%d pset_iffreq=%08x\n",
		if_freq, (unsigned)pset_iffreq);

	ret = rtl2832_wr_demod_reg(dev, DVBT_EN_BBIN, en_bbin);
	if (ret)
		return ret;

	ret = rtl2832_wr_demod_reg(dev, DVBT_PSET_IFFREQ, pset_iffreq);

	return ret;
}

static int rtl2832_init(struct dvb_frontend *fe)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;
	struct dtv_frontend_properties *c = &dev->fe.dtv_property_cache;
	const struct rtl2832_reg_value *init;
	int i, ret, len;
	/* initialization values for the demodulator registers */
	struct rtl2832_reg_value rtl2832_initial_regs[] = {
		{DVBT_AD_EN_REG,		0x1},
		{DVBT_AD_EN_REG1,		0x1},
		{DVBT_RSD_BER_FAIL_VAL,		0x2800},
		{DVBT_MGD_THD0,			0x10},
		{DVBT_MGD_THD1,			0x20},
		{DVBT_MGD_THD2,			0x20},
		{DVBT_MGD_THD3,			0x40},
		{DVBT_MGD_THD4,			0x22},
		{DVBT_MGD_THD5,			0x32},
		{DVBT_MGD_THD6,			0x37},
		{DVBT_MGD_THD7,			0x39},
		{DVBT_EN_BK_TRK,		0x0},
		{DVBT_EN_CACQ_NOTCH,		0x0},
		{DVBT_AD_AV_REF,		0x2a},
		{DVBT_REG_PI,			0x6},
		{DVBT_PIP_ON,			0x0},
		{DVBT_CDIV_PH0,			0x8},
		{DVBT_CDIV_PH1,			0x8},
		{DVBT_SCALE1_B92,		0x4},
		{DVBT_SCALE1_B93,		0xb0},
		{DVBT_SCALE1_BA7,		0x78},
		{DVBT_SCALE1_BA9,		0x28},
		{DVBT_SCALE1_BAA,		0x59},
		{DVBT_SCALE1_BAB,		0x83},
		{DVBT_SCALE1_BAC,		0xd4},
		{DVBT_SCALE1_BB0,		0x65},
		{DVBT_SCALE1_BB1,		0x43},
		{DVBT_KB_P1,			0x1},
		{DVBT_KB_P2,			0x4},
		{DVBT_KB_P3,			0x7},
		{DVBT_K1_CR_STEP12,		0xa},
		{DVBT_REG_GPE,			0x1},
		{DVBT_SERIAL,			0x0},
		{DVBT_CDIV_PH0,			0x9},
		{DVBT_CDIV_PH1,			0x9},
		{DVBT_MPEG_IO_OPT_2_2,		0x0},
		{DVBT_MPEG_IO_OPT_1_0,		0x0},
		{DVBT_TRK_KS_P2,		0x4},
		{DVBT_TRK_KS_I2,		0x7},
		{DVBT_TR_THD_SET2,		0x6},
		{DVBT_TRK_KC_I2,		0x5},
		{DVBT_CR_THD_SET2,		0x1},
	};

	dev_dbg(&client->dev, "\n");

	for (i = 0; i < ARRAY_SIZE(rtl2832_initial_regs); i++) {
		ret = rtl2832_wr_demod_reg(dev, rtl2832_initial_regs[i].reg,
			rtl2832_initial_regs[i].value);
		if (ret)
			goto err;
	}

	/* load tuner specific settings */
	dev_dbg(&client->dev, "load settings for tuner=%02x\n",
		dev->pdata->tuner);
	switch (dev->pdata->tuner) {
	case RTL2832_TUNER_FC0012:
	case RTL2832_TUNER_FC0013:
		len = ARRAY_SIZE(rtl2832_tuner_init_fc0012);
		init = rtl2832_tuner_init_fc0012;
		break;
	case RTL2832_TUNER_TUA9001:
		len = ARRAY_SIZE(rtl2832_tuner_init_tua9001);
		init = rtl2832_tuner_init_tua9001;
		break;
	case RTL2832_TUNER_E4000:
		len = ARRAY_SIZE(rtl2832_tuner_init_e4000);
		init = rtl2832_tuner_init_e4000;
		break;
	case RTL2832_TUNER_R820T:
	case RTL2832_TUNER_R828D:
		len = ARRAY_SIZE(rtl2832_tuner_init_r820t);
		init = rtl2832_tuner_init_r820t;
		break;
	default:
		ret = -EINVAL;
		goto err;
	}

	for (i = 0; i < len; i++) {
		ret = rtl2832_wr_demod_reg(dev, init[i].reg, init[i].value);
		if (ret)
			goto err;
	}

	/*
	 * r820t NIM code does a software reset here at the demod -
	 * may not be needed, as there's already a software reset at
	 * set_params()
	 */
#if 1
	/* soft reset */
	ret = rtl2832_wr_demod_reg(dev, DVBT_SOFT_RST, 0x1);
	if (ret)
		goto err;

	ret = rtl2832_wr_demod_reg(dev, DVBT_SOFT_RST, 0x0);
	if (ret)
		goto err;
#endif
	/* init stats here in order signal app which stats are supported */
	c->cnr.len = 1;
	c->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	c->post_bit_error.len = 1;
	c->post_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	c->post_bit_count.len = 1;
	c->post_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	/* start statistics polling */
	schedule_delayed_work(&dev->stat_work, msecs_to_jiffies(2000));
	dev->sleeping = false;

	return ret;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_sleep(struct dvb_frontend *fe)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;

	dev_dbg(&client->dev, "\n");
	dev->sleeping = true;
	/* stop statistics polling */
	cancel_delayed_work_sync(&dev->stat_work);
	dev->fe_status = 0;
	return 0;
}

static int rtl2832_get_tune_settings(struct dvb_frontend *fe,
	struct dvb_frontend_tune_settings *s)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;

	dev_dbg(&client->dev, "\n");
	s->min_delay_ms = 1000;
	s->step_size = fe->ops.info.frequency_stepsize * 2;
	s->max_drift = (fe->ops.info.frequency_stepsize * 2) + 1;
	return 0;
}

static int rtl2832_set_frontend(struct dvb_frontend *fe)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret, i, j;
	u64 bw_mode, num, num2;
	u32 resamp_ratio, cfreq_off_ratio;
	static u8 bw_params[3][32] = {
	/* 6 MHz bandwidth */
		{
		0xf5, 0xff, 0x15, 0x38, 0x5d, 0x6d, 0x52, 0x07, 0xfa, 0x2f,
		0x53, 0xf5, 0x3f, 0xca, 0x0b, 0x91, 0xea, 0x30, 0x63, 0xb2,
		0x13, 0xda, 0x0b, 0xc4, 0x18, 0x7e, 0x16, 0x66, 0x08, 0x67,
		0x19, 0xe0,
		},

	/*  7 MHz bandwidth */
		{
		0xe7, 0xcc, 0xb5, 0xba, 0xe8, 0x2f, 0x67, 0x61, 0x00, 0xaf,
		0x86, 0xf2, 0xbf, 0x59, 0x04, 0x11, 0xb6, 0x33, 0xa4, 0x30,
		0x15, 0x10, 0x0a, 0x42, 0x18, 0xf8, 0x17, 0xd9, 0x07, 0x22,
		0x19, 0x10,
		},

	/*  8 MHz bandwidth */
		{
		0x09, 0xf6, 0xd2, 0xa7, 0x9a, 0xc9, 0x27, 0x77, 0x06, 0xbf,
		0xec, 0xf4, 0x4f, 0x0b, 0xfc, 0x01, 0x63, 0x35, 0x54, 0xa7,
		0x16, 0x66, 0x08, 0xb4, 0x19, 0x6e, 0x19, 0x65, 0x05, 0xc8,
		0x19, 0xe0,
		},
	};


	dev_dbg(&client->dev, "frequency=%u bandwidth_hz=%u inversion=%u\n",
		c->frequency, c->bandwidth_hz, c->inversion);

	/* program tuner */
	if (fe->ops.tuner_ops.set_params)
		fe->ops.tuner_ops.set_params(fe);

	/* PIP mode related */
	ret = rtl2832_wr_regs(dev, 0x92, 1, "\x00\x0f\xff", 3);
	if (ret)
		goto err;

	/* If the frontend has get_if_frequency(), use it */
	if (fe->ops.tuner_ops.get_if_frequency) {
		u32 if_freq;

		ret = fe->ops.tuner_ops.get_if_frequency(fe, &if_freq);
		if (ret)
			goto err;

		ret = rtl2832_set_if(fe, if_freq);
		if (ret)
			goto err;
	}

	switch (c->bandwidth_hz) {
	case 6000000:
		i = 0;
		bw_mode = 48000000;
		break;
	case 7000000:
		i = 1;
		bw_mode = 56000000;
		break;
	case 8000000:
		i = 2;
		bw_mode = 64000000;
		break;
	default:
		dev_err(&client->dev, "invalid bandwidth_hz %u\n",
			c->bandwidth_hz);
		ret = -EINVAL;
		goto err;
	}

	for (j = 0; j < sizeof(bw_params[0]); j++) {
		ret = rtl2832_wr_regs(dev, 0x1c+j, 1, &bw_params[i][j], 1);
		if (ret)
			goto err;
	}

	/* calculate and set resample ratio
	* RSAMP_RATIO = floor(CrystalFreqHz * 7 * pow(2, 22)
	*	/ ConstWithBandwidthMode)
	*/
	num = dev->pdata->clk * 7;
	num *= 0x400000;
	num = div_u64(num, bw_mode);
	resamp_ratio =  num & 0x3ffffff;
	ret = rtl2832_wr_demod_reg(dev, DVBT_RSAMP_RATIO, resamp_ratio);
	if (ret)
		goto err;

	/* calculate and set cfreq off ratio
	* CFREQ_OFF_RATIO = - floor(ConstWithBandwidthMode * pow(2, 20)
	*	/ (CrystalFreqHz * 7))
	*/
	num = bw_mode << 20;
	num2 = dev->pdata->clk * 7;
	num = div_u64(num, num2);
	num = -num;
	cfreq_off_ratio = num & 0xfffff;
	ret = rtl2832_wr_demod_reg(dev, DVBT_CFREQ_OFF_RATIO, cfreq_off_ratio);
	if (ret)
		goto err;

	/* soft reset */
	ret = rtl2832_wr_demod_reg(dev, DVBT_SOFT_RST, 0x1);
	if (ret)
		goto err;

	ret = rtl2832_wr_demod_reg(dev, DVBT_SOFT_RST, 0x0);
	if (ret)
		goto err;

	return ret;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_get_frontend(struct dvb_frontend *fe)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret;
	u8 buf[3];

	if (dev->sleeping)
		return 0;

	ret = rtl2832_rd_regs(dev, 0x3c, 3, buf, 2);
	if (ret)
		goto err;

	ret = rtl2832_rd_reg(dev, 0x51, 3, &buf[2]);
	if (ret)
		goto err;

	dev_dbg(&client->dev, "TPS=%*ph\n", 3, buf);

	switch ((buf[0] >> 2) & 3) {
	case 0:
		c->modulation = QPSK;
		break;
	case 1:
		c->modulation = QAM_16;
		break;
	case 2:
		c->modulation = QAM_64;
		break;
	}

	switch ((buf[2] >> 2) & 1) {
	case 0:
		c->transmission_mode = TRANSMISSION_MODE_2K;
		break;
	case 1:
		c->transmission_mode = TRANSMISSION_MODE_8K;
	}

	switch ((buf[2] >> 0) & 3) {
	case 0:
		c->guard_interval = GUARD_INTERVAL_1_32;
		break;
	case 1:
		c->guard_interval = GUARD_INTERVAL_1_16;
		break;
	case 2:
		c->guard_interval = GUARD_INTERVAL_1_8;
		break;
	case 3:
		c->guard_interval = GUARD_INTERVAL_1_4;
		break;
	}

	switch ((buf[0] >> 4) & 7) {
	case 0:
		c->hierarchy = HIERARCHY_NONE;
		break;
	case 1:
		c->hierarchy = HIERARCHY_1;
		break;
	case 2:
		c->hierarchy = HIERARCHY_2;
		break;
	case 3:
		c->hierarchy = HIERARCHY_4;
		break;
	}

	switch ((buf[1] >> 3) & 7) {
	case 0:
		c->code_rate_HP = FEC_1_2;
		break;
	case 1:
		c->code_rate_HP = FEC_2_3;
		break;
	case 2:
		c->code_rate_HP = FEC_3_4;
		break;
	case 3:
		c->code_rate_HP = FEC_5_6;
		break;
	case 4:
		c->code_rate_HP = FEC_7_8;
		break;
	}

	switch ((buf[1] >> 0) & 7) {
	case 0:
		c->code_rate_LP = FEC_1_2;
		break;
	case 1:
		c->code_rate_LP = FEC_2_3;
		break;
	case 2:
		c->code_rate_LP = FEC_3_4;
		break;
	case 3:
		c->code_rate_LP = FEC_5_6;
		break;
	case 4:
		c->code_rate_LP = FEC_7_8;
		break;
	}

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;
	struct i2c_client *client = dev->client;
	int ret;
	u32 tmp;

	dev_dbg(&client->dev, "\n");

	*status = 0;
	if (dev->sleeping)
		return 0;

	ret = rtl2832_rd_demod_reg(dev, DVBT_FSM_STAGE, &tmp);
	if (ret)
		goto err;

	if (tmp == 11) {
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER |
				FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;
	}
	/* TODO find out if this is also true for rtl2832? */
	/*else if (tmp == 10) {
		*status |= FE_HAS_SIGNAL | FE_HAS_CARRIER |
				FE_HAS_VITERBI;
	}*/

	dev->fe_status = *status;
	return ret;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	/* report SNR in resolution of 0.1 dB */
	if (c->cnr.stat[0].scale == FE_SCALE_DECIBEL)
		*snr = div_s64(c->cnr.stat[0].svalue, 100);
	else
		*snr = 0;

	return 0;
}

static int rtl2832_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct rtl2832_dev *dev = fe->demodulator_priv;

	*ber = (dev->post_bit_error - dev->post_bit_error_prev);
	dev->post_bit_error_prev = dev->post_bit_error;

	return 0;
}

static void rtl2832_stat_work(struct work_struct *work)
{
	struct rtl2832_dev *dev = container_of(work, struct rtl2832_dev, stat_work.work);
	struct i2c_client *client = dev->client;
	struct dtv_frontend_properties *c = &dev->fe.dtv_property_cache;
	int ret, tmp;
	u8 u8tmp, buf[2];
	u16 u16tmp;

	dev_dbg(&client->dev, "\n");

	/* CNR */
	if (dev->fe_status & FE_HAS_VITERBI) {
		unsigned hierarchy, constellation;
		#define CONSTELLATION_NUM 3
		#define HIERARCHY_NUM 4
		static const u32 constant[CONSTELLATION_NUM][HIERARCHY_NUM] = {
			{85387325, 85387325, 85387325, 85387325},
			{86676178, 86676178, 87167949, 87795660},
			{87659938, 87659938, 87885178, 88241743},
		};

		ret = rtl2832_bulk_read(client, 0x33c, &u8tmp, 1);
		if (ret)
			goto err;

		constellation = (u8tmp >> 2) & 0x03; /* [3:2] */
		if (constellation > CONSTELLATION_NUM - 1)
			goto err_schedule_delayed_work;

		hierarchy = (u8tmp >> 4) & 0x07; /* [6:4] */
		if (hierarchy > HIERARCHY_NUM - 1)
			goto err_schedule_delayed_work;

		ret = rtl2832_bulk_read(client, 0x40c, buf, 2);
		if (ret)
			goto err;

		u16tmp = buf[0] << 8 | buf[1] << 0;
		if (u16tmp)
			tmp = (constant[constellation][hierarchy] -
			       intlog10(u16tmp)) / ((1 << 24) / 10000);
		else
			tmp = 0;

		dev_dbg(&client->dev, "cnr raw=%u\n", u16tmp);

		c->cnr.stat[0].scale = FE_SCALE_DECIBEL;
		c->cnr.stat[0].svalue = tmp;
	} else {
		c->cnr.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	}

	/* BER */
	if (dev->fe_status & FE_HAS_LOCK) {
		ret = rtl2832_bulk_read(client, 0x34e, buf, 2);
		if (ret)
			goto err;

		u16tmp = buf[0] << 8 | buf[1] << 0;
		dev->post_bit_error += u16tmp;
		dev->post_bit_count += 1000000;

		dev_dbg(&client->dev, "ber errors=%u total=1000000\n", u16tmp);

		c->post_bit_error.stat[0].scale = FE_SCALE_COUNTER;
		c->post_bit_error.stat[0].uvalue = dev->post_bit_error;
		c->post_bit_count.stat[0].scale = FE_SCALE_COUNTER;
		c->post_bit_count.stat[0].uvalue = dev->post_bit_count;
	} else {
		c->post_bit_error.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
		c->post_bit_count.stat[0].scale = FE_SCALE_NOT_AVAILABLE;
	}

err_schedule_delayed_work:
	schedule_delayed_work(&dev->stat_work, msecs_to_jiffies(2000));
	return;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
}

/*
 * I2C gate/mux/repeater logic
 * We must use unlocked __i2c_transfer() here (through regmap) because of I2C
 * adapter lock is already taken by tuner driver.
 * There is delay mechanism to avoid unneeded I2C gate open / close. Gate close
 * is delayed here a little bit in order to see if there is sequence of I2C
 * messages sent to same I2C bus.
 */
static void rtl2832_i2c_gate_work(struct work_struct *work)
{
	struct rtl2832_dev *dev = container_of(work, struct rtl2832_dev, i2c_gate_work.work);
	struct i2c_client *client = dev->client;
	int ret;

	/* close gate */
	ret = rtl2832_update_bits(dev->client, 0x101, 0x08, 0x00);
	if (ret)
		goto err;

	dev->i2c_gate_state = false;

	return;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return;
}

static int rtl2832_select(struct i2c_adapter *adap, void *mux_priv, u32 chan_id)
{
	struct rtl2832_dev *dev = mux_priv;
	struct i2c_client *client = dev->client;
	int ret;

	/* terminate possible gate closing */
	cancel_delayed_work(&dev->i2c_gate_work);

	if (dev->i2c_gate_state == chan_id)
		return 0;

	/*
	 * chan_id 1 is muxed adapter demod provides and chan_id 0 is demod
	 * itself. We need open gate when request is for chan_id 1. On that case
	 * I2C adapter lock is already taken and due to that we will use
	 * regmap_update_bits() which does not lock again I2C adapter.
	 */
	if (chan_id == 1)
		ret = regmap_update_bits(dev->regmap, 0x101, 0x08, 0x08);
	else
		ret = rtl2832_update_bits(dev->client, 0x101, 0x08, 0x00);
	if (ret)
		goto err;

	dev->i2c_gate_state = chan_id;

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_deselect(struct i2c_adapter *adap, void *mux_priv,
			    u32 chan_id)
{
	struct rtl2832_dev *dev = mux_priv;

	schedule_delayed_work(&dev->i2c_gate_work, usecs_to_jiffies(100));
	return 0;
}

static struct dvb_frontend_ops rtl2832_ops = {
	.delsys = { SYS_DVBT },
	.info = {
		.name = "Realtek RTL2832 (DVB-T)",
		.frequency_min	  = 174000000,
		.frequency_max	  = 862000000,
		.frequency_stepsize = 166667,
		.caps = FE_CAN_FEC_1_2 |
			FE_CAN_FEC_2_3 |
			FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 |
			FE_CAN_FEC_7_8 |
			FE_CAN_FEC_AUTO |
			FE_CAN_QPSK |
			FE_CAN_QAM_16 |
			FE_CAN_QAM_64 |
			FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_RECOVER |
			FE_CAN_MUTE_TS
	 },

	.init = rtl2832_init,
	.sleep = rtl2832_sleep,

	.get_tune_settings = rtl2832_get_tune_settings,

	.set_frontend = rtl2832_set_frontend,
	.get_frontend = rtl2832_get_frontend,

	.read_status = rtl2832_read_status,
	.read_snr = rtl2832_read_snr,
	.read_ber = rtl2832_read_ber,

	.i2c_gate_ctrl = rtl2832_i2c_gate_ctrl,
};

/*
 * We implement own I2C access routines for regmap in order to get manual access
 * to I2C adapter lock, which is needed for I2C mux adapter.
 */
static int rtl2832_regmap_read(void *context, const void *reg_buf,
			       size_t reg_size, void *val_buf, size_t val_size)
{
	struct i2c_client *client = context;
	int ret;
	struct i2c_msg msg[2] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = reg_size,
			.buf = (u8 *)reg_buf,
		}, {
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = val_size,
			.buf = val_buf,
		}
	};

	ret = __i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) {
		dev_warn(&client->dev, "i2c reg read failed %d\n", ret);
		if (ret >= 0)
			ret = -EREMOTEIO;
		return ret;
	}
	return 0;
}

static int rtl2832_regmap_write(void *context, const void *data, size_t count)
{
	struct i2c_client *client = context;
	int ret;
	struct i2c_msg msg[1] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = count,
			.buf = (u8 *)data,
		}
	};

	ret = __i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) {
		dev_warn(&client->dev, "i2c reg write failed %d\n", ret);
		if (ret >= 0)
			ret = -EREMOTEIO;
		return ret;
	}
	return 0;
}

static int rtl2832_regmap_gather_write(void *context, const void *reg,
				       size_t reg_len, const void *val,
				       size_t val_len)
{
	struct i2c_client *client = context;
	int ret;
	u8 buf[256];
	struct i2c_msg msg[1] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1 + val_len,
			.buf = buf,
		}
	};

	buf[0] = *(u8 const *)reg;
	memcpy(&buf[1], val, val_len);

	ret = __i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) {
		dev_warn(&client->dev, "i2c reg write failed %d\n", ret);
		if (ret >= 0)
			ret = -EREMOTEIO;
		return ret;
	}
	return 0;
}

static struct dvb_frontend *rtl2832_get_dvb_frontend(struct i2c_client *client)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "\n");
	return &dev->fe;
}

static struct i2c_adapter *rtl2832_get_i2c_adapter_(struct i2c_client *client)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "\n");
	return dev->i2c_adapter_tuner;
}

static struct i2c_adapter *rtl2832_get_private_i2c_adapter_(struct i2c_client *client)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "\n");
	return dev->i2c_adapter;
}

static int rtl2832_enable_slave_ts(struct i2c_client *client)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);
	int ret;

	dev_dbg(&client->dev, "\n");

	ret = rtl2832_wr_regs(dev, 0x0c, 1, "\x5f\xff", 2);
	if (ret)
		goto err;

	ret = rtl2832_wr_demod_reg(dev, DVBT_PIP_ON, 0x1);
	if (ret)
		goto err;

	ret = rtl2832_wr_reg(dev, 0xbc, 0, 0x18);
	if (ret)
		goto err;

	ret = rtl2832_wr_reg(dev, 0x22, 0, 0x01);
	if (ret)
		goto err;

	ret = rtl2832_wr_reg(dev, 0x26, 0, 0x1f);
	if (ret)
		goto err;

	ret = rtl2832_wr_reg(dev, 0x27, 0, 0xff);
	if (ret)
		goto err;

	ret = rtl2832_wr_regs(dev, 0x92, 1, "\x7f\xf7\xff", 3);
	if (ret)
		goto err;

	/* soft reset */
	ret = rtl2832_wr_demod_reg(dev, DVBT_SOFT_RST, 0x1);
	if (ret)
		goto err;

	ret = rtl2832_wr_demod_reg(dev, DVBT_SOFT_RST, 0x0);
	if (ret)
		goto err;

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct rtl2832_platform_data *pdata = client->dev.platform_data;
	struct i2c_adapter *i2c = client->adapter;
	struct rtl2832_dev *dev;
	int ret;
	u8 tmp;
	static const struct regmap_bus regmap_bus = {
		.read = rtl2832_regmap_read,
		.write = rtl2832_regmap_write,
		.gather_write = rtl2832_regmap_gather_write,
		.val_format_endian_default = REGMAP_ENDIAN_NATIVE,
	};
	static const struct regmap_range_cfg regmap_range_cfg[] = {
		{
			.selector_reg     = 0x00,
			.selector_mask    = 0xff,
			.selector_shift   = 0,
			.window_start     = 0,
			.window_len       = 0x100,
			.range_min        = 0 * 0x100,
			.range_max        = 5 * 0x100,
		},
	};
	static const struct regmap_config regmap_config = {
		.reg_bits    =  8,
		.val_bits    =  8,
		.max_register = 5 * 0x100,
		.ranges = regmap_range_cfg,
		.num_ranges = ARRAY_SIZE(regmap_range_cfg),
	};

	dev_dbg(&client->dev, "\n");

	/* allocate memory for the internal state */
	dev = kzalloc(sizeof(struct rtl2832_dev), GFP_KERNEL);
	if (dev == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	/* setup the state */
	i2c_set_clientdata(client, dev);
	dev->client = client;
	dev->pdata = client->dev.platform_data;
	if (pdata->config) {
		dev->pdata->clk = pdata->config->xtal;
		dev->pdata->tuner = pdata->config->tuner;
	}
	dev->sleeping = true;
	INIT_DELAYED_WORK(&dev->i2c_gate_work, rtl2832_i2c_gate_work);
	INIT_DELAYED_WORK(&dev->stat_work, rtl2832_stat_work);
	/* create regmap */
	dev->regmap = regmap_init(&client->dev, &regmap_bus, client,
				  &regmap_config);
	if (IS_ERR(dev->regmap)) {
		ret = PTR_ERR(dev->regmap);
		goto err_kfree;
	}
	/* create muxed i2c adapter for demod itself */
	dev->i2c_adapter = i2c_add_mux_adapter(i2c, &i2c->dev, dev, 0, 0, 0,
			rtl2832_select, NULL);
	if (dev->i2c_adapter == NULL) {
		ret = -ENODEV;
		goto err_regmap_exit;
	}

	/* check if the demod is there */
	ret = rtl2832_rd_reg(dev, 0x00, 0x0, &tmp);
	if (ret)
		goto err_i2c_del_mux_adapter;

	/* create muxed i2c adapter for demod tuner bus */
	dev->i2c_adapter_tuner = i2c_add_mux_adapter(i2c, &i2c->dev, dev,
			0, 1, 0, rtl2832_select, rtl2832_deselect);
	if (dev->i2c_adapter_tuner == NULL) {
		ret = -ENODEV;
		goto err_i2c_del_mux_adapter;
	}

	/* create dvb_frontend */
	memcpy(&dev->fe.ops, &rtl2832_ops, sizeof(struct dvb_frontend_ops));
	dev->fe.demodulator_priv = dev;

	/* setup callbacks */
	pdata->get_dvb_frontend = rtl2832_get_dvb_frontend;
	pdata->get_i2c_adapter = rtl2832_get_i2c_adapter_;
	pdata->get_private_i2c_adapter = rtl2832_get_private_i2c_adapter_;
	pdata->enable_slave_ts = rtl2832_enable_slave_ts;

	dev_info(&client->dev, "Realtek RTL2832 successfully attached\n");
	return 0;
err_i2c_del_mux_adapter:
	i2c_del_mux_adapter(dev->i2c_adapter);
err_regmap_exit:
	regmap_exit(dev->regmap);
err_kfree:
	kfree(dev);
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int rtl2832_remove(struct i2c_client *client)
{
	struct rtl2832_dev *dev = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "\n");

	cancel_delayed_work_sync(&dev->i2c_gate_work);

	i2c_del_mux_adapter(dev->i2c_adapter_tuner);

	i2c_del_mux_adapter(dev->i2c_adapter);

	regmap_exit(dev->regmap);

	kfree(dev);

	return 0;
}

static const struct i2c_device_id rtl2832_id_table[] = {
	{"rtl2832", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, rtl2832_id_table);

static struct i2c_driver rtl2832_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "rtl2832",
	},
	.probe		= rtl2832_probe,
	.remove		= rtl2832_remove,
	.id_table	= rtl2832_id_table,
};

module_i2c_driver(rtl2832_driver);

MODULE_AUTHOR("Thomas Mair <mair.thomas86@gmail.com>");
MODULE_DESCRIPTION("Realtek RTL2832 DVB-T demodulator driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.5");
