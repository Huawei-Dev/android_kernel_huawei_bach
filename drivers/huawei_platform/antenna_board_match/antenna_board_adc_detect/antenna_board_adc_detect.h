/*
 * drivers/antenna_board_match/antenna_board_adc_detect.h
 *
 * huawei antenna board detect driver
 *
*/

#ifndef _ANTENNA_BOARD_ADC_DETECT
#define _ANTENNA_BOARD_ADC_DETECT
#include <linux/device.h>


struct antenna_device_info {
    struct device *dev;
};

enum antenna_type {
    ANTENNA_BOARD_ADC_DETECT,
};

#endif
