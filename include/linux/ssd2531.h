/*
 * include/linux/ssd2531.h - platform data structure for f75375s sensor
 *
 * Copyright (C) 2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SSD2531_H
#define _LINUX_SSD2531_H

#define SSD2531_NAME "ssd2531-ts"

struct ssd2531_platform_data {
    int pin_reset;
	int (*power)(int on);	/* Only valid in first array entry */
};

#endif /* _LINUX_SSD2531_H */
