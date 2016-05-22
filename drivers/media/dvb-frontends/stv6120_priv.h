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

#ifndef __STV6120_PRIV_H
#define __STV6120_PRIV_H

#define FE_ERROR				0
#define FE_NOTICE				1
#define FE_INFO					2
#define FE_DEBUG				3
#define FE_DEBUGREG				4

#define POW2(a)					(a * a)
#define MAKEWORD16(a, b)			(((a) << 8) | (b))

#define INRANGE(val, x, y)		(((x <= val) && (val <= y)) ||		\
					 ((y <= val) && (val <= x)) ? 1 : 0)

#define LSB(x)					((x & 0xff))
#define MSB(y)					((y >> 8) & 0xff)

#define TRIALS					10
#define R_DIV(__div)				(1 << (__div + 1))
#define REFCLOCK_kHz				(stv6120->config->refclk /    1000)
#define REFCLOCK_MHz				(stv6120->config->refclk / 1000000)

struct stv6120_state {
	struct i2c_adapter		*i2c;
	const struct stv6120_config	*config;
	u8                              regs[8];
	u8				tuner;
	u32				frequency;
	u32				bandwidth;
	u8				bbgain;
};

#endif /* __STV6120_PRIV_H */
