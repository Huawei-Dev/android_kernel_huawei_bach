#ifndef __LP8556_H__
#define __LP8556_H__

/*Enable LED*/
#define LED_ENABLE_VALUE  0X0F

/*Only read EPROMs once on initial power-up
 * PWM input and brightness register
 * Backlight disabled and chip turned off*/
#define DEVICE_CONTROL_VALUE_1 0X86

/*Set the lk brightness for 100*/
#define BRIGHTNESS_CONTROL_VALUE 0X64

/*Set boost switching frequency 1250kHZ*/
#define BOOST_FREQ_VALUE 0X40

/*The CURRENT_LSB_CFG0_VALUE and the CURRENT_LSB_CFG1_VALUE
 * to scale the current for 50mA*/
#define CURRENT_LSB_CFG0_VALUE 0XFF
#define CURRENT_LSB_CFG1_VALUE 0X3F

/*Select PWM output phase configuration of 4
 *and the PWM frequency for 19232HZ*/
#define CURRENT_LSB_CFG5_VALUE 0X2B

/*Only read EPROMs once on initial power-up
 * PWM input and Brightness register
 * Backing enabled and chip turned on*/
#define DEVICE_CONTROL_VALUE_2 0X87

#endif
