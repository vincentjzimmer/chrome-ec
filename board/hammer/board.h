/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Hammer configuration */

#ifndef __CROS_EC_BOARD_H
#define __CROS_EC_BOARD_H

/* TODO: Remove CONFIG_SYSTEM_UNLOCKED prior to building MP FW. */
#define CONFIG_SYSTEM_UNLOCKED

/* 48 MHz SYSCLK clock frequency */
#define CPU_CLOCK 48000000

/*
 * Flash layout: we redefine the sections offsets and sizes as we want to
 * include a rollback region, and will use RO/RW regions of different sizes.
 */
#undef _IMAGE_SIZE
#undef CONFIG_ROLLBACK_OFF
#undef CONFIG_ROLLBACK_SIZE
#undef CONFIG_FLASH_PSTATE
#undef CONFIG_FW_PSTATE_SIZE
#undef CONFIG_FW_PSTATE_OFF
#undef CONFIG_SHAREDLIB_SIZE
#undef CONFIG_RO_MEM_OFF
#undef CONFIG_RO_STORAGE_OFF
#undef CONFIG_RO_SIZE
#undef CONFIG_RW_MEM_OFF
#undef CONFIG_RW_STORAGE_OFF
#undef CONFIG_RW_SIZE
#undef CONFIG_EC_PROTECTED_STORAGE_OFF
#undef CONFIG_EC_PROTECTED_STORAGE_SIZE
#undef CONFIG_EC_WRITABLE_STORAGE_OFF
#undef CONFIG_EC_WRITABLE_STORAGE_SIZE
#undef CONFIG_WP_STORAGE_OFF
#undef CONFIG_WP_STORAGE_SIZE

#define _IMAGE_SIZE		(CONFIG_FLASH_SIZE / 2)

/* EC rollback protection block */
#define CONFIG_ROLLBACK_OFF _IMAGE_SIZE
#define CONFIG_ROLLBACK_SIZE CONFIG_FLASH_BANK_SIZE

#define CONFIG_FLASH_PSTATE
/* Do not use a dedicated PSTATE bank */
#undef CONFIG_FLASH_PSTATE_BANK

#define CONFIG_SHAREDLIB_SIZE	0

#define CONFIG_RO_MEM_OFF	0
#define CONFIG_RO_STORAGE_OFF	0
#define CONFIG_RO_SIZE		_IMAGE_SIZE
#define CONFIG_RW_MEM_OFF	(_IMAGE_SIZE + CONFIG_ROLLBACK_SIZE)
#define CONFIG_RW_STORAGE_OFF	0
#define CONFIG_RW_SIZE		(_IMAGE_SIZE - CONFIG_ROLLBACK_SIZE)

#define CONFIG_EC_PROTECTED_STORAGE_OFF		CONFIG_RO_MEM_OFF
#define CONFIG_EC_PROTECTED_STORAGE_SIZE	CONFIG_RO_SIZE
#define CONFIG_EC_WRITABLE_STORAGE_OFF		CONFIG_RW_MEM_OFF
#define CONFIG_EC_WRITABLE_STORAGE_SIZE		CONFIG_RW_SIZE

#define CONFIG_WP_STORAGE_OFF		CONFIG_EC_PROTECTED_STORAGE_OFF
#define CONFIG_WP_STORAGE_SIZE		CONFIG_EC_PROTECTED_STORAGE_SIZE

/* The UART console is on USART1 (PA9/PA10) */
#undef CONFIG_UART_CONSOLE
#define CONFIG_UART_CONSOLE 1

/* Optional features */
#define CONFIG_LOW_POWER_IDLE
#define CONFIG_FORCE_CONSOLE_RESUME
#define CONFIG_STM_HWTIMER32
#define CONFIG_HW_CRC

/* USB Configuration */
#define CONFIG_USB
#define CONFIG_USB_PID 0x5022
#define CONFIG_STREAM_USB
#define CONFIG_USB_UPDATE
#define CONFIG_USB_HID
#define CONFIG_USB_HID_KEYBOARD
#define CONFIG_USB_HID_TOUCHPAD

#undef CONFIG_USB_MAXPOWER_MA
#define CONFIG_USB_MAXPOWER_MA 100

#define CONFIG_USB_REMOTE_WAKEUP
#define CONFIG_USB_SUSPEND

#define CONFIG_USB_SERIALNO
/* TODO(drinkcat): Replace this by proper serial number. Note that according to
 * USB standard, we must either unset this (iSerialNumber = 0), or have a unique
 * serial number per device.
 */
#define DEFAULT_SERIALNO "Uninitialized"

/* USB interface indexes (use define rather than enum to expand them) */
#define USB_IFACE_UPDATE	0
#define USB_IFACE_HID_KEYBOARD	1
#define USB_IFACE_HID_TOUCHPAD	2
#define USB_IFACE_COUNT		3

/* USB endpoint indexes (use define rather than enum to expand them) */
#define USB_EP_CONTROL		0
#define USB_EP_UPDATE		1
#define USB_EP_HID_KEYBOARD	2
#define USB_EP_HID_TOUCHPAD	3
#define USB_EP_COUNT		4

/* Optional features */
#define CONFIG_BOARD_PRE_INIT
#define CONFIG_WATCHDOG_HELP

/* No lid switch */
#undef CONFIG_LID_SWITCH

/* Keyboard output port list */
#define CONFIG_KEYBOARD_DEBUG
#define KB_OUT_PORT_LIST GPIO_A, GPIO_B, GPIO_C, GPIO_F

/* Enable I2C */
#define CONFIG_I2C
#define CONFIG_I2C_MASTER
#define I2C_PORT_MASTER 0

/* Enable PWM */
#define CONFIG_PWM

/* Sign and switch to RW partition on boot. */
#define CONFIG_RWSIG
#define CONFIG_RWSIG_TYPE_RWSIG
#define CONFIG_RSA
#define CONFIG_SHA256
#define CONFIG_RSA_KEY_SIZE 2048
#define CONFIG_RSA_EXPONENT_3

/*
 * Add rollback protection, and independent RW region protection.
 */
#define CONFIG_ROLLBACK
#define CONFIG_FLASH_PROTECT_RW

/* Enable elan trackpad driver */
#define CONFIG_TOUCHPAD_ELAN
#define CONFIG_TOUCHPAD_I2C_PORT 0
#define CONFIG_TOUCHPAD_I2C_ADDR (0x15 << 1)

#ifndef __ASSEMBLER__

/* Timer selection */
#define TIM_CLOCK32 2
#define TIM_WATCHDOG 16
#define TIM_KBLIGHT 17

#include "gpio_signal.h"

/* USB string indexes */
enum usb_strings {
	USB_STR_DESC = 0,
	USB_STR_VENDOR,
	USB_STR_PRODUCT,
	USB_STR_SERIALNO,
	USB_STR_VERSION,
	USB_STR_UPDATE_NAME,
	USB_STR_COUNT
};

enum pwm_channel {
	PWM_CH_KBLIGHT = 0,
	/* Number of PWM channels */
	PWM_CH_COUNT
};

#endif /* !__ASSEMBLER__ */
#endif /* __CROS_EC_BOARD_H */
