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

#ifndef OV2659_H
#define OV2659_H

#include <linux/types.h>
#include <mach/camera.h>

extern struct ov2659_reg ov2659_regs;

enum ov2659_width {
	WORD_LEN,
	BYTE_LEN
};

struct ov2659_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum ov2659_width width;
	unsigned short mdelay_time;
};

struct ov2659_reg {
   	const struct ov2659_i2c_reg_conf *init_tbl;
	uint16_t init_tbl_size;
   	const struct ov2659_i2c_reg_conf *preview_tbl;
	uint16_t preview_tbl_size;
   	const struct ov2659_i2c_reg_conf *snapshot_tbl;
	uint16_t snapshot_tbl_size;

	const struct ov2659_i2c_reg_conf *awb_tbl;
	uint16_t awb_tbl_size;
	const struct ov2659_i2c_reg_conf *mwb_cloudy_tbl;
	uint16_t mwb_cloudy_tbl_size;
	const struct ov2659_i2c_reg_conf *mwb_day_light_tbl;
	uint16_t mwb_day_light_tbl_size;
	const struct ov2659_i2c_reg_conf *mwb_fluorescent_tbl;
	uint16_t mwb_fluorescent_tbl_size;
	const struct ov2659_i2c_reg_conf *mwb_incandescent_tbl;
	uint16_t mwb_incandescent_tbl_size;

	const struct ov2659_i2c_reg_conf *effect_off_tbl;
	uint16_t effect_off_tbl_size;
	const struct ov2659_i2c_reg_conf *effect_mono_tbl;
	uint16_t effect_mono_tbl_size;
	const struct ov2659_i2c_reg_conf *effect_sepia_tbl;
	uint16_t effect_sepia_tbl_size;
	const struct ov2659_i2c_reg_conf *effect_negative_tbl;
	uint16_t effect_negative_tbl_size;
	const struct ov2659_i2c_reg_conf *effect_solarize_tbl;
	uint16_t effect_solarize_tbl_size;
	const struct ov2659_i2c_reg_conf *effect_bluish_tbl;
	uint16_t effect_bluish_tbl_size;
	const struct ov2659_i2c_reg_conf *effect_greenish_tbl;
	uint16_t effect_greenish_tbl_size;
	const struct ov2659_i2c_reg_conf *effect_reddish_tbl;
	uint16_t effect_reddish_tbl_size;

	const struct ov2659_i2c_reg_conf *scene_auto_tbl;
	uint16_t scene_auto_tbl_size;
	const struct ov2659_i2c_reg_conf *scene_night_tbl;
	uint16_t scene_night_tbl_size;

};

#endif /* OV2659_H */
