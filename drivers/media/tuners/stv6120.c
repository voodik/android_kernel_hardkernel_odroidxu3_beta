/*
 * STV6120 Silicon tuner driver
 *
 * Copyright (C) Chris Leee <updatelee@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "dvb_frontend.h"

#include "stv6110x.h"
#include "stv6120_reg.h"
#include "stv6120.h"
#include "stv6120_priv.h"

static unsigned int verbose;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "Set Verbosity level");

struct SLookup {
	s16 Value;
	u16 RegValue;
};

static struct SLookup Gain_RFAGC_LookUp[] = {                
	/*Gain *100dB*/   /*reg*/
	{	7429		,	0        },  /*	74.61 dB	*/
	{	7368		,	18711	 },  /*	74.43 dB	*/
	{	7214		,	23432	 },  /*	73.36 dB	*/
	{	7090		,	25123	 },  /*	72.4  dB	*/
	{	6988		,	26305	 },  /*	71.47 dB	*/
	{	6897		,	27100	 },  /*	70.47 dB	*/
	{	6809		,	27741	 },  /*	69.5  dB	*/
	{	6728		,	28271	 },  /*	68.52 dB	*/
	{	6645		,	28737	 },  /*	67.52 dB	*/
	{	6571		,	29120	 },  /*	66.53 dB	*/
	{	6494		,	29504	 },  /*	65.54 dB	*/
	{	6416		,	29857	 },  /*	64.55 dB	*/
	{	6341		,	30180	 },  /*	63.55 dB	*/
	{	6263		,	30490	 },  /*	62.56 dB	*/
	{	6179		,	30815	 },  /*	61.57 dB	*/
	{	6101		,	31088	 },  /*	60.58 dB	*/
	{	6028		,	31345	 },  /*	59.57 dB	*/
	{	5956		,	31600	 },  /*	58.59 dB	*/
	{	5883		,	31840	 },  /*	57.6  dB	*/
	{	5801		,	32096	 },  /*	56.6  dB	*/
	{	5730		,	32320	 },  /*	55.59 dB	*/
	{	5659		,	32544	 },  /*	54.59 dB	*/
	{	5582		,	32752	 },  /*	53.61 dB	*/
	{	5498		,	32960	 },  /*	52.61 dB	*/
	{	5414		,	33184	 },  /*	51.61 dB	*/
	{	5340		,	33392	 },  /*	50.62 dB	*/
	{	5271		,	33584	 },  /*	49.61 dB	*/
	{	5198		,	33775	 },  /*	48.61 dB	*/
	{	5125		,	33967	 },  /*	47.62 dB	*/
	{	5048		,	34160	 },  /*	46.62 dB	*/
	{	4963		,	34352	 },  /*	45.62 dB	*/
	{	4884		,	34543	 },  /*	44.62 dB	*/
	{	4820		,	34719	 },  /*	43.62 dB	*/
	{	4740		,	34910	 },  /*	42.62 dB	*/
	{	4666		,	35103	 },  /*	41.62 dB	*/
	{	4582		,	35295	 },  /*	40.62 dB	*/
	{	4500		,	35488	 },  /*	39.62 dB	*/
	{	4426		,	35680	 },  /*	38.62 dB	*/
	{	4365		,	35870	 },  /*	37.62 dB	*/
	{	4279		,	36095	 },  /*	36.62 dB	*/
	{	4113		,	36289	 },  /*	35.62 dB	*/
	{	4020		,	36500	 },  /*	34.62 dB	*/
	{	3930		,	36704	 },  /*	33.62 dB	*/
	{	3838		,	36912	 },  /*	32.62 dB	*/
	{	3738		,	37152	 },  /*	31.62 dB	*/
	{	3648		,	37375	 },  /*	30.62 dB	*/
	{	3544		,	37600	 },  /*	29.62 dB	*/
	{	3458		,	37823	 },  /*	28.62 dB	*/
	{	3358		,	38048	 },  /*	27.62 dB	*/
	{	3281		,	38240	 },  /*	26.62 dB	*/
	{	3191		,	38479	 },  /*	25.62 dB	*/
	{	3091		,	38720	 },  /*	24.62 dB	*/
	{	2993		,	38976	 },  /*	23.63 dB	*/
	{	2900		,	39226	 },  /*	22.63 dB	*/
	{	2792		,	39520	 },  /*	21.62 dB	*/
	{	2692		,	39792	 },  /*	20.62 dB	*/
	{	2592    	,	40064	 },  /*	19.62 dB	*/
	{	2497		,	40351	 },  /*	18.62 dB	*/
	{	2392		,	40640	 },  /*	17.62 dB	*/
	{	2290		,	40976	 },  /*	16.62 dB	*/
	{	2189		,	41295	 },  /*	15.62 dB	*/
	{	2088		,	41631	 },  /*	14.62 dB	*/
	{	1999		,	41934	 },  /*	13.62 dB	*/
	{	1875		,	42354	 },  /*	12.62 dB	*/
	{	1764	   	,	42815	 },  /*	11.62 dB	*/
	{	1637	   	,	43263	 },  /*	10.62 dB	*/	
	{	1537	   	,	43743    },  /*	9.62  dB	*/		
	{	1412		,	44288    },  /*	8.62  dB	*/	
	{	1291		,	44913    },  /*	7.62  dB	*/		
	{	1188	  	,	45712    },  /*	6.62  dB	*/	
	{	1080    	,	46720    },  /*	5.63  dB	*/	
	{	976		,       48164	 },  /*	4.63  dB	*/	
	{	930	    	,	50816    },  /*	3.63  dB	*/	
	{	898	    	,	65534    },  /*	2.94  dB	*/	
	{	880	    	,	65535    }  /*	2.95  dB	*/	
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static s32 TableLookup(struct SLookup *Table, int TableSize, u16 RegValue)
{
	s32 Gain;
	s32 RegDiff;
	int imin = 0;
	int imax = TableSize - 1;
	int i;
	
	// Assumes Table[0].RegValue < Table[imax].RegValue 
	if( RegValue <= Table[0].RegValue )
		Gain = Table[0].Value;
	else if( RegValue >= Table[imax].RegValue )
		Gain = Table[imax].Value;
	else {
		while(imax-imin > 1) {
			i = (imax + imin) / 2;
			if ((Table[imin].RegValue <= RegValue) &&
			    (RegValue <= Table[i].RegValue) )
				imax = i;
			else
				imin = i;
		}
		RegDiff = Table[imax].RegValue - Table[imin].RegValue;
		Gain = Table[imin].Value;
		if (RegDiff != 0)
			Gain += ((s32) (RegValue - Table[imin].RegValue) *
			    (s32)(Table[imax].Value - Table[imin].Value))/(RegDiff);
	}
	return Gain;
}

/* Max transfer size done by I2C transfer functions */
#define MAX_XFER_SIZE  64

static void extract_mask_pos(u32 label, u8 *mask, u8 *pos)
{
	u8 position = 0, i = 0;

	(*mask) = label & 0xff;

	while ((position == 0) && (i < 8)) {
		position = ((*mask) >> i) & 0x01;
		i++;
	}

	(*pos) = (i - 1);
}

static int stv6120_read_regs(struct stv6120_state *state, u16 reg, u8 *data, u8 len)
{
	int ret;
	u8 b0[] = { reg & 0xff };
	struct i2c_msg msg[] = {
		{ .addr = state->config->addr, .flags = 0,        .buf = b0,   .len = 1 },
		{ .addr = state->config->addr, .flags = I2C_M_RD, .buf = data, .len = len }
	};

	ret = i2c_transfer(state->i2c, msg, 2);
	if (ret != 2) {
		pr_err("I2C Error\n");
		return -EREMOTEIO;
	}
	return 0;
}

static u8 stv6120_read_reg(struct stv6120_state *state, u16 reg)
{
	u8 data = 0x00;

	stv6120_read_regs(state, reg, &data, 1);
	return data;
}

static u8 stv6120_read_field(struct stv6120_state *state, u32 label)
{
	u8 mask, pos, data;

	extract_mask_pos(label, &mask, &pos);

	data = stv6120_read_reg(state, label >> 16);
	data = (data & mask) >> pos;

	return data;
}

static int stv6120_write_regs(struct stv6120_state *state, u16 reg, u8 *data, u8 len)
{
	int ret;
	u8 buf[MAX_XFER_SIZE];

	struct i2c_msg msg = {
		.addr  = state->config->addr,
		.flags = 0,
		.buf   = buf,
		.len   = len + 1
	};

	buf[0] = reg & 0xff;
	memcpy(&buf[1], data, len);

	ret = i2c_transfer(state->i2c, &msg, 1);
	if (ret != 1) {
		pr_err("I2C Error\n");
		return -EREMOTEIO;
	}

	return 0;
}

static int stv6120_write_reg(struct stv6120_state *state, u16 reg, u8 data)
{
	return stv6120_write_regs(state, reg, &data, 1);
}

static int stv6120_write_field(struct stv6120_state *state, u32 label, u8 data)
{
	u8 reg, mask, pos;

	reg = stv6120_read_reg(state, (label >> 16) & 0xffff);
	extract_mask_pos(label, &mask, &pos);

	data = mask & (data << pos);
	reg = (reg & (~mask)) | data;

	return stv6120_write_reg(state, (label >> 16) & 0xffff, reg);
}

static int stv6120_init(struct dvb_frontend *fe)
{
	struct stv6120_state *state = fe->tuner_priv;

	//pr_warn("%s: tuner: %d\n", __func__, state->tuner);

	stv6120_write_reg(state, RSTV6120_STAT1,  0x0E);
	stv6120_write_reg(state, RSTV6120_CTRL10, 0x03);
	stv6120_write_reg(state, RSTV6120_CTRL15, 0x0D);
	stv6120_write_reg(state, RSTV6120_STAT2,  0x0E);
	stv6120_write_reg(state, RSTV6120_CTRL18, 0x00);
	stv6120_write_reg(state, RSTV6120_CTRL19, 0x00);
	stv6120_write_reg(state, RSTV6120_CTRL20, 0x4C);
	stv6120_write_reg(state, RSTV6120_CTRL21, 0x00);
	stv6120_write_reg(state, RSTV6120_CTRL22, 0x00);
	stv6120_write_reg(state, RSTV6120_CTRL23, 0x4C);
	stv6120_write_reg(state, RSTV6120_CTRL10, 0x13);
	stv6120_write_reg(state, RSTV6120_CTRL10, 0x1B);

	return 0;
}

static int stv6120_set_cutoff(struct dvb_frontend *fe, u32 frequency)
{
	struct stv6120_state *state = fe->tuner_priv;

	u8 cfhf;

	if (frequency < 6796000)
		cfhf = 0;
	if (frequency < 5828000)
		cfhf = 1;
	if (frequency < 4778000)
		cfhf = 2;
	if (frequency < 4118000)
		cfhf = 3;
	if (frequency < 3513000)
		cfhf = 4;
	if (frequency < 3136000)
		cfhf = 5;
	if (frequency < 2794000)
		cfhf = 6;
	if (frequency < 2562000)
		cfhf = 7;
	if (frequency < 2331000)
		cfhf = 8;
	if (frequency < 2169000)
		cfhf = 9;
	if (frequency < 2006000)
		cfhf = 10;
	if (frequency < 1890000)
		cfhf = 11;
	if (frequency < 1771000)
		cfhf = 12;
	if (frequency < 1680000)
		cfhf = 13;
	if (frequency < 1586000)
		cfhf = 14;
	if (frequency < 1514000)
		cfhf = 15;
	if (frequency < 1433000)
		cfhf = 16;
	if (frequency < 1374000)
		cfhf = 17;
	if (frequency < 1310000)
		cfhf = 18;
	if (frequency < 1262000)
		cfhf = 19;
	if (frequency < 1208000)
		cfhf = 20;
	if (frequency < 1167000)
		cfhf = 21;
	if (frequency < 1122000)
		cfhf = 22;
	if (frequency < 1087000)
		cfhf = 23;
	if (frequency < 1049000)
		cfhf = 24;
	if (frequency < 1018000)
		cfhf = 25;
	if (frequency < 983000)
		cfhf = 26;
	if (frequency < 956000)
		cfhf = 27;
	if (frequency < 926000)
		cfhf = 28;
	if (frequency < 902000)
		cfhf = 29;
	if (frequency < 875000)
		cfhf = 30;
	if (frequency < 854000)
		cfhf = 31;

	STV6120_WRITE_FIELD(state, CFHF, cfhf);

	//pr_warn("%s: tuner: %d, freq: %d CFHF: %d\n", __func__, state->tuner, frequency, cfhf);

	return 0;
}

static int stv6120_set_frequency(struct dvb_frontend *fe, u32 frequency)
{
	struct stv6120_state *state = fe->tuner_priv;
	const struct stv6120_config *config = state->config;

	u32 Fxtl, Fvco, FRdiv, N, F;
	u8  P, PDiv, R, ICP, i;

	//pr_warn("%s: tuner: %d, freq: %d\n", __func__, state->tuner, frequency);
	
	STV6120_WRITE_FIELD(state, BBGAIN, state->bbgain/2);

	stv6120_set_cutoff(fe, frequency);

	if (frequency >= 250000) {
		P = 16;
		PDiv = 3;
	}
	if (frequency >= 299000) {
		P = 8;
		PDiv = 2;
	}
	if (frequency >= 596000) {
		P = 4;
		PDiv = 1;
	}
	if (frequency >= 1191000) {
		P = 2;
		PDiv = 0;
	}

	Fvco = frequency * P;

	if (Fvco >= 2380000) {
		ICP = 0;
	}
	if (Fvco >= 2473000) {
		ICP = 1;
	}
	if (Fvco >= 2701000) {
		ICP = 2;
	}
	if (Fvco >= 3022000) {
		ICP = 3; /* 3 or 4 */
	}
	if (Fvco >= 3388000) {
		ICP = 5;
	}
	if (Fvco >= 3846000) {
		ICP = 6;
	}
	if (Fvco >= 4395000) {
		ICP = 7;
	}

	/* fLO = fVCO / P = (fXTAL / R) * (N + F / 2^18) / P */
	Fxtl  = config->refclk / 1000; /* 30mhz */
	R     = config->clk_div;          /* 2     */
	FRdiv = Fxtl / R;
	N     = Fvco / FRdiv;
	F     = ((Fvco % FRdiv) * 0x40000) / FRdiv;

//	pr_info("%s: Fvco:%02x Fxtl:%02x R:%02x FRdiv:%02x ICP:%02x PDiv:%02x\n", __func__, Fvco, Fxtl, R, FRdiv, ICP, PDiv);
//	pr_info("%s: N:%08x F:%08x\n", __func__, N, F);

	STV6120_WRITE_FIELD(state, ICP, ICP);
	STV6120_WRITE_FIELD(state, PDIV, PDiv);
	STV6120_WRITE_FIELD(state, NDIV_LSB, (N & 0xFF));
	STV6120_WRITE_FIELD(state, NDIV_MSB, ((N>>8) & 0x01));
	STV6120_WRITE_FIELD(state, F_H, ((F>>15) & 0x07));
	STV6120_WRITE_FIELD(state, F_M, ((F>>7) & 0xFF));
	STV6120_WRITE_FIELD(state, F_L, (F & 0x7F));
	STV6120_WRITE_FIELD(state, CALVCOSTRT, 1); /* VCO Auto Calibration */

	for (i = 0; !STV6120_READ_FIELD(state, LOCK); i++) {
		if (i > 10) {
			pr_err("%s: VCO Lock Failed...\n", __func__);
			return 1;
		}
		msleep(10);
	}
	
	state->frequency = frequency;

	return 0;
}

static int stv6120_get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	struct stv6120_state *state = fe->tuner_priv;
	
	*frequency = state->frequency;

	return 0;
}

static int stv6120_set_bandwidth(struct dvb_frontend *fe, u32 bandwidth)
{
	struct stv6120_state *state = fe->tuner_priv;
	u8 lpf;
	s32 i;

	if ((bandwidth/2) > 36000000) /* F[4:0] BW/2 max =31+5=36 mhz for F=31 */
		lpf = 31;
	else if ((bandwidth/2) < 5000000) /* BW/2 min = 5Mhz for F=0 */
		lpf = 0;
	else /* if 5 < BW/2 < 36 */
		lpf = (bandwidth/2)/1000000 - 5;

	//pr_warn("%s: tuner: %d, bw: %d, lpf: %d\n", __func__, state->tuner, bandwidth, lpf);

	STV6120_WRITE_FIELD(state, CF, lpf); /* Set the LPF value */
	STV6120_WRITE_FIELD(state, CALRCSTRT, 1); /* Start LPF auto calibration */

	i = 0;
	while((i < 10) && (STV6120_READ_FIELD(state, CALRCSTRT) != 0)) {
		msleep(10); /* wait for LPF auto calibration */
		i++;
	}

	state->bandwidth = bandwidth;
	//pr_warn("%s: LPF:%d\n", __func__, lpf);
	return 0;
}

static int stv6120_get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	struct stv6120_state *state = fe->tuner_priv;
	
	*bandwidth = state->bandwidth;

	return 0;
}
static int stv6120_set_mode(struct dvb_frontend *fe, enum tuner_mode mode)
{
	struct stv6120_state *state = fe->tuner_priv;

	switch (mode) {
//	case TUNER_WAKE:
//		pr_warn("%s: TUNER_WAKE\n", __func__);
//		if (state->tuner == TUNER_1) {
//			stv6120_write_field(state, SYN_1, 0);
//			stv6120_write_field(state, SDOFF_1, 1);
//			stv6120_write_field(state, PATHON_1, 0);
//		}
//		if (state->tuner == TUNER_2) {
//			stv6120_write_field(state, SYN_2, 0);
//			stv6120_write_field(state, SDOFF_1, 1);
//			stv6120_write_field(state, PATHON_1, 0);
//		}
		break;
	case TUNER_WAKE:
		pr_warn("%s: tuner: %d TUNER_WAKE\n", __func__, state->tuner);

//		stv6120_write_field(state, LNABON, 1);
//		stv6120_write_field(state, LNACON, 1);

//		stv6120_write_field(state, SYN_1, 1);
//		stv6120_write_field(state, SDOFF_1, 0);
//		stv6120_write_field(state, PATHON_1, 1);
//		stv6120_write_field(state, SYN_2, 1);
//		stv6120_write_field(state, SDOFF_2, 0);
//		stv6120_write_field(state, PATHON_2, 1);
		break;
	case TUNER_SLEEP: // This actually TUNER_WAKE up the tuner, fix this
		pr_warn("%s: tuner: %d TUNER_SLEEP\n", __func__, state->tuner);

//		stv6120_write_field(state, SYN_1, 1);
//		stv6120_write_field(state, SDOFF_1, 0);
//		stv6120_write_field(state, PATHON_1, 1);
//		stv6120_write_field(state, SYN_2, 1);
//		stv6120_write_field(state, SDOFF_2, 0);
//		stv6120_write_field(state, PATHON_2, 1);
		break;
	}

	return 0;
}

static int stv6120_sleep(struct dvb_frontend *fe)
{
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	if (fe->tuner_priv)
		return stv6120_set_mode(fe, TUNER_SLEEP);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return 0;
}

static int stv6120_get_status(struct dvb_frontend *fe, u32 *status)
{
	struct stv6120_state *state = fe->tuner_priv;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	if (STV6120_READ_FIELD(state, LOCK)) {
		*status = TUNER_PHASELOCKED;
	} else {
		*status = 0;
	}

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return 0;
}

static int stv6120_get_rf_strength(struct dvb_frontend *fe, u16 *st)
{
	struct stv6120_state *state = fe->tuner_priv;
	s32 gain = 1, ref_bbgain = 12, tilt = 6;
	u32 freq;

	gain = TableLookup(Gain_RFAGC_LookUp, ARRAY_SIZE(Gain_RFAGC_LookUp), *st);

	gain += 100 * (state->bbgain - ref_bbgain); 	
	
	freq = state->frequency / 10000;
	
	if (freq<159)
		gain -= 200; /* HMR filter 2dB gain compensation below freq=1590MHz */	
	
	gain-=(((freq-155)*tilt)/12)*10; 

	return 0;
}

static int stv6120_release(struct dvb_frontend *fe)
{
	struct stv6120_state *state = fe->tuner_priv;

	fe->tuner_priv = NULL;
	kfree(state);

	return 0;
}

static int stv6120_set_params(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);
	
	stv6120_set_bandwidth(fe, c->bandwidth_hz);
	stv6120_set_frequency(fe, c->frequency);
	
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return 0;
}

static struct dvb_tuner_ops stv6120_ops = {
	.info = {
		.name		= "STV6120 Silicon Tuner",
		.frequency_min	=  250000,
		.frequency_max	= 2150000,
		.frequency_step	= 1000,
	},
	.init		= stv6120_init,
	.release	= stv6120_release,
	.sleep          = stv6120_sleep,
	.set_frequency	= stv6120_set_frequency,
	.get_frequency	= stv6120_get_frequency,
	.set_bandwidth	= stv6120_set_bandwidth,
	.get_bandwidth	= stv6120_get_bandwidth,
	.set_params	= stv6120_set_params,
	.get_status	= stv6120_get_status,  
	.get_rf_strength = stv6120_get_rf_strength,
};

extern struct dvb_frontend *stv6120_attach(struct dvb_frontend *fe, const struct stv6120_config *config, struct i2c_adapter *i2c)
{
	struct stv6120_state *state;

	state = kzalloc(sizeof(struct stv6120_state), GFP_KERNEL);
	if (!state)
		return NULL;

	state->i2c    = i2c;
	state->config = config;
	state->tuner  = config->tuner;
	state->bbgain  = config->bbgain;

	fe->tuner_priv    = state;
	fe->ops.tuner_ops = stv6120_ops;

	pr_info("%s: Attaching stv6120, tuner: %d, bbgain : %d dB\n", __func__, state->tuner, state->bbgain);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	stv6120_set_mode(fe, TUNER_WAKE);
	stv6120_init(fe);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);

	return fe;
}
EXPORT_SYMBOL(stv6120_attach);

MODULE_AUTHOR("Chris Lee");
MODULE_DESCRIPTION("STV6120 Silicon tuner");
MODULE_LICENSE("GPL");
