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

#ifndef __STV6120_H
#define __STV6120_H

struct stv6120_config {
	u8	addr;
	u32	refclk;
	u8	clk_div; /* divisor value for the output clock */
	u8	tuner;
	u8      bbgain;
};

enum tuner_number {
	TUNER_1,
	TUNER_2,
};

#if IS_REACHABLE(CONFIG_DVB_STV6120)
extern struct dvb_frontend *stv6120_attach(struct dvb_frontend *fe, const struct stv6120_config *config, struct i2c_adapter *i2c);
#else
extern struct dvb_frontend *stv6120_attach(struct dvb_frontend *fe, const struct stv6120_config *config, struct i2c_adapter *i2c)
{
	pr_info("%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif /* CONFIG_DVB_STV6120 */
#endif /* __STV6120_H */
