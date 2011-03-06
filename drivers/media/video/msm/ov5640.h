/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef OV5640_H
#define OV5640_H

#include <linux/types.h>
#include <mach/camera.h>

extern struct ov5640_reg ov5640_regs;

enum ov5640_width {
	WORD_LEN,
	BYTE_LEN
};

struct ov5640_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum ov5640_width width;
	unsigned short mdelay_time;
};

struct ov5640_reg {
	const struct register_address_value_pair *init_settings;
	uint16_t init_settings_size;
	const struct register_address_value_pair *af_settings;
	uint16_t af_settings_size;
	const struct register_address_value_pair *qxga_xga_settings;
	uint16_t qxga_xga_settings_size;
	const struct register_address_value_pair *xga_qxga_settings;
	uint16_t xga_qxga_settings_size;
};

#endif /* OV5640_H */
