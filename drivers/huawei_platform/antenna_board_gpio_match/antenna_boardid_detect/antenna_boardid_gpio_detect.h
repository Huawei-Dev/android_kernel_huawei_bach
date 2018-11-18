/*
 * drivers/antenna_boardid_detect/antenna_boardid_gpio_detect.h
 *
 * huawei antenna boardid gpio detect driver
 *
*/

#ifndef _ANTENNA_BOARDID_GPIO_DETECT
#define _ANTENNA_BOARDID_GPIO_DETECT
#include <linux/device.h>

struct antenna_device_info {
    struct device *dev;
};

enum antenna_type {
    ANTENNA_BOARDID_GPIO_DETECT,
    ANTENNA_BOARDID_GPIO_STATUS,
};

enum {
    STATUS_DOWN_DOWN = 0,    /* ID1 down, ID2 down, boardid version is AL00. */
    STATUS_DOWN_UP,          /* ID1 down, ID2 UP, boardid version is TL10. */
    STATUS_UP_DOWN,          /* ID1 up, ID2 down, boardid version is LX1. */
    STATUS_UP_UP,            /* ID1 up, ID2 up, boardid version is LX2. */
    STATUS_DOWN_DOWN_OTHER,  /* ID1 down, ID2 down, ID1 and ID2 connection, boardid version is LX3. */
    STATUS_UP_UP_OTHER,      /* ID1 up, ID2 up, ID1 and ID2 connection,reserved. */
    STATUS_ERROR,
};

#define ANATENNA_DETECT_SUCCEED 1
#define ANATENNA_DETECT_FAIL    0

#endif
