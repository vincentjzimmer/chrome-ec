/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* NPCX-specific SIB module for Chrome EC */

#ifndef __CROS_EC_SYSTEM_CHIP_H
#define __CROS_EC_SYSTEM_CHIP_H

/* Indices for battery-backed ram (BBRAM) data position */
enum bbram_data_index {
	BBRM_DATA_INDEX_SCRATCHPAD = 0,        /* General-purpose scratchpad */
	BBRM_DATA_INDEX_SAVED_RESET_FLAGS = 4, /* Saved reset flags */
	BBRM_DATA_INDEX_WAKE = 8,	       /* Wake reasons for hibernate */
	BBRM_DATA_INDEX_PD0 = 12,	       /* USB-PD saved port0 state */
	BBRM_DATA_INDEX_PD1 = 13,	       /* USB-PD saved port1 state */
	BBRM_DATA_INDEX_VBNVCNTXT = 16,	       /* VbNvContext for ARM arch */
	BBRM_DATA_INDEX_RAMLOG = 32,	       /* RAM log for Booter */
};

/* Issue a watchdog reset*/
void system_watchdog_reset(void);
/* Check reset cause and return reset flags */
void system_check_reset_cause(void);

/* Begin flash address for hibernate utility; defined in linker script */
extern unsigned int __flash_lpfw_start;
/* End flash address for hibernate utility; defined in linker script */
extern unsigned int __flash_lpfw_end;

/* Begin flash address for little FW; defined in linker script */
extern unsigned int __flash_lplfw_start;
/* End flash address for little FW; defined in linker script */
extern unsigned int __flash_lplfw_end;

#endif /* __CROS_EC_SYSTEM_CHIP_H */
