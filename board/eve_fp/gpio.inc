/*
 * Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Declare symbolic names for all the GPIOs that we care about.
 * Note: Those with interrupt handlers must be declared first. */
GPIO_INT(FPS_INT,  PIN(A, 1), GPIO_INT_BOTH, fps_event)
GPIO_INT(SPI1_NSS, PIN(A, 4), GPIO_INT_BOTH | GPIO_PULL_UP,  spi_event)

/* Outputs */
GPIO(AP_INT,     PIN(A, 2), GPIO_ODR_HIGH | GPIO_PULL_UP)
GPIO(FP_RST_ODL, PIN(A, 3), GPIO_ODR_HIGH)
GPIO(SPI3_NSS,   PIN(A, 15), GPIO_OUT_HIGH)

/* Unimplemented signals which we need to emulate for now */
UNIMPLEMENTED(ENTERING_RW)
UNIMPLEMENTED(WP_L)

/* USART1: PA9/10 */
ALTERNATE(PIN_MASK(A, 0x0600), GPIO_ALT_F7, MODULE_UART, 0)
/* SPI1 slave from PCH: PA4/5/6/7 */
ALTERNATE(PIN_MASK(A, 0x00f0), GPIO_ALT_F5, MODULE_SPI,  0)
/* SPI3 master to sensor: PB3/4/5 */
ALTERNATE(PIN_MASK(B, 0x0038), GPIO_ALT_F6, MODULE_SPI_MASTER,  0)
