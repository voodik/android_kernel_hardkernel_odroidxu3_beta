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

#ifndef __STV6120_REG_H
#define __STV6120_REG_H

/*CTRL1*/
#define RSTV6120_CTRL1  0x0000
#define FSTV6120_K  0x000000f8
#define FSTV6120_RDIV  0x00000004
#define FSTV6120_OSHP  0x00000002
#define FSTV6120_MCLKDIV  0x00000001

/*CTRL2*/
#define RSTV6120_CTRL2  0x0001
#define FSTV6120_P1_DCLOOPOFF  0x00010080
#define FSTV6120_P1_SDOFF  0x00010040
#define FSTV6120_P1_SYN  0x00010020
#define FSTV6120_P1_REFOUTSEL  0x00010010
#define FSTV6120_P1_BBGAIN  0x0001000f

/*CTRL3*/
#define RSTV6120_CTRL3  0x0002
#define FSTV6120_P1_NDIV_LSB  0x000200ff

/*CTRL4*/
#define RSTV6120_CTRL4  0x0003
#define FSTV6120_P1_F_L  0x000300fe
#define FSTV6120_P1_NDIV_MSB  0x00030001

/*CTRL5*/
#define RSTV6120_CTRL5  0x0004
#define FSTV6120_P1_F_M  0x000400ff

/*CTRL6*/
#define RSTV6120_CTRL6  0x0005
#define FSTV6120_P1_ICP  0x00050070
#define FSTV6120_P1_VCOILOW  0x00050008
#define FSTV6120_P1_F_H  0x00050007

/*CTRL7*/
#define RSTV6120_CTRL7  0x0006
#define FSTV6120_P1_RCCLFOFF  0x00060080
#define FSTV6120_P1_PDIV  0x00060060
#define FSTV6120_P1_CF  0x0006001f

/*CTRL8*/
#define RSTV6120_CTRL8  0x0007
#define FSTV6120_TCAL  0x000700c0
#define FSTV6120_P1_CALTIME  0x00070020
#define FSTV6120_P1_CFHF  0x0007001f

/*STAT1*/
#define RSTV6120_STAT1  0x0008
#define FSTV6120_P1_CALVCOSTRT  0x00080004
#define FSTV6120_P1_CALRCSTRT  0x00080002
#define FSTV6120_P1_LOCK  0x00080001

/*CTRL9*/
#define RSTV6120_CTRL9  0x0009
#define FSTV6120_RFSEL_2  0x0009000c
#define FSTV6120_RFSEL_1  0x00090003

/*CTRL10*/
#define RSTV6120_CTRL10  0x000a
#define FSTV6120_LNADON  0x000a0020
#define FSTV6120_LNACON  0x000a0010
#define FSTV6120_LNABON  0x000a0008
#define FSTV6120_LNAAON  0x000a0004
#define FSTV6120_PATHON_2  0x000a0002
#define FSTV6120_PATHON_1  0x000a0001

/*CTRL11*/
#define RSTV6120_CTRL11  0x000b
#define FSTV6120_P2_DCLOOPOFF  0x000b0080
#define FSTV6120_P2_SDOFF  0x000b0040
#define FSTV6120_P2_SYN  0x000b0020
#define FSTV6120_P2_REFOUTSEL  0x000b0010
#define FSTV6120_P2_BBGAIN  0x000b000f

/*CTRL12*/
#define RSTV6120_CTRL12  0x000c
#define FSTV6120_P2_NDIV_LSB  0x000c00ff

/*CTRL13*/
#define RSTV6120_CTRL13  0x000d
#define FSTV6120_P2_F_L  0x000d00fe
#define FSTV6120_P2_NDIV_MSB  0x000d0001

/*CTRL14*/
#define RSTV6120_CTRL14  0x000e
#define FSTV6120_P2_F_M  0x000e00ff

/*CTRL15*/
#define RSTV6120_CTRL15  0x000f
#define FSTV6120_P2_ICP  0x000f0070
#define FSTV6120_P2_VCOILOW  0x000f0008
#define FSTV6120_P2_F_H  0x000f0007

/*CTRL16*/
#define RSTV6120_CTRL16  0x0010
#define FSTV6120_P2_RCCLFOFF  0x00100080
#define FSTV6120_P2_PDIV  0x00100060
#define FSTV6120_P2_CF  0x0010001f

/*CTRL17*/
#define RSTV6120_CTRL17  0x0011
#define FSTV6120_P2_CALTIME  0x00110020
#define FSTV6120_P2_CFHF  0x0011001f

/*STAT2*/
#define RSTV6120_STAT2  0x0012
#define FSTV6120_P2_CALVCOSTRT  0x00120004
#define FSTV6120_P2_CALRCSTRT  0x00120002
#define FSTV6120_P2_LOCK  0x00120001

/*CTRL18*/
#define RSTV6120_CTRL18  0x0013
#define FSTV6120_TEST1  0x001300ff

/*CTRL19*/
#define RSTV6120_CTRL19  0x0014
#define FSTV6120_TEST2  0x001400ff

/*CTRL20*/
#define RSTV6120_CTRL20  0x0015
#define FSTV6120_TEST3  0x001500ff

/*CTRL21*/
#define RSTV6120_CTRL21  0x0016
#define FSTV6120_TEST4  0x001600ff

/*CTRL22*/
#define RSTV6120_CTRL22  0x0017
#define FSTV6120_TEST5  0x001700ff

/*CTRL23*/
#define RSTV6120_CTRL23  0x0018
#define FSTV6120_TEST6  0x001800ff

#define STV6120_WRITE_FIELD(__state, __reg, __data)                         \
	(((__state)->tuner == TUNER_1) ?                                           \
		stv6120_write_field(__state, FSTV6120_P1_##__reg, __data) : \
		stv6120_write_field(__state, FSTV6120_P2_##__reg, __data))

#define STV6120_READ_FIELD(__state, __reg)                         \
	(((__state)->tuner == TUNER_1) ?                                           \
		stv6120_read_field(__state, FSTV6120_P1_##__reg) : \
		stv6120_read_field(__state, FSTV6120_P2_##__reg))

#endif /* __STV6120_REG_H */
