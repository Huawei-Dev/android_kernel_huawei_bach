/*
** =============================================================================
** Copyright (c) 2016  Texas Instruments Inc.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; version 2.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
** Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
** File:
**     tas2560-misc.h
**
** Description:
**     header file for tas2560-misc.c
**
** =============================================================================
*/

#ifndef _TAS2560_MISC_H
#define _TAS2560_MISC_H

#define	TIAUDIO_CMD_REG_WITE			1
#define	TIAUDIO_CMD_REG_READ			2
#define	TIAUDIO_CMD_DEBUG_ON			3
#define	TIAUDIO_CMD_CALIBRATION			7
#define	TIAUDIO_CMD_SAMPLERATE			8
#define	TIAUDIO_CMD_BITRATE				9
#define	TIAUDIO_CMD_DACVOLUME			10
#define	TIAUDIO_CMD_SPEAKER				11

#define	TAS2560_MAGIC_NUMBER	0x32353630	/* '2560' */

enum reg_value_count {
	REG_COUNT_0 = 0,
	REG_COUNT_1 = 1,
	REG_COUNT_2 = 2,
	REG_COUNT_3 = 3,
	REG_COUNT_4 = 4,
	REG_COUNT_5 = 5
};

#define	SMARTPA_SPK_DAC_VOLUME	 			_IOWR(TAS2560_MAGIC_NUMBER, 1, unsigned long)
#define	SMARTPA_SPK_POWER_ON 				_IOWR(TAS2560_MAGIC_NUMBER, 2, unsigned long)
#define	SMARTPA_SPK_POWER_OFF 				_IOWR(TAS2560_MAGIC_NUMBER, 3, unsigned long)
#define	SMARTPA_SPK_SET_SAMPLERATE		 	_IOWR(TAS2560_MAGIC_NUMBER, 7, unsigned long)
#define	SMARTPA_SPK_SET_BITRATE			 	_IOWR(TAS2560_MAGIC_NUMBER, 8, unsigned long)
#define	SMARTPA_SPK_DUMP_REGISTER	 		_IOWR(TAS2560_MAGIC_NUMBER, 9, __u32)

int tas2560_register_misc(struct tas2560_priv *pTAS2560);
int tas2560_deregister_misc(struct tas2560_priv *pTAS2560);

#endif /* _TAS2560_MISC_H */
