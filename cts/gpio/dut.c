/* Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "watchdog.h"
#include "uart.h"
#include "timer.h"
#include "watchdog.h"
#include "dut_common.h"
#include "cts_common.h"

enum cts_rc sync_test(void)
{
	return CTS_RC_SUCCESS;
}

enum cts_rc set_high_test(void)
{
	gpio_set_flags(GPIO_OUTPUT_TEST, GPIO_ODR_LOW);
	gpio_set_level(GPIO_OUTPUT_TEST, 1);
	msleep(READ_WAIT_TIME_MS*2);
	return CTS_RC_SUCCESS;
}

enum cts_rc set_low_test(void)
{
	gpio_set_flags(GPIO_OUTPUT_TEST, GPIO_ODR_LOW);
	gpio_set_level(GPIO_OUTPUT_TEST, 0);
	msleep(READ_WAIT_TIME_MS*2);
	return CTS_RC_SUCCESS;
}

enum cts_rc read_high_test(void)
{
	int level;

	gpio_set_flags(GPIO_INPUT_TEST, GPIO_INPUT | GPIO_PULL_UP);
	msleep(READ_WAIT_TIME_MS);
	level = gpio_get_level(GPIO_INPUT_TEST);
	if (level)
		return CTS_RC_SUCCESS;
	else
		return CTS_RC_FAILURE;
}

enum cts_rc read_low_test(void)
{
	int level;

	gpio_set_flags(GPIO_INPUT_TEST, GPIO_INPUT | GPIO_PULL_UP);
	msleep(READ_WAIT_TIME_MS);
	level = gpio_get_level(GPIO_INPUT_TEST);
	if (!level)
		return CTS_RC_SUCCESS;
	else
		return CTS_RC_FAILURE;
}

enum cts_rc od_read_high_test(void)
{
	int level;

	gpio_set_flags(GPIO_OUTPUT_TEST, GPIO_ODR_HIGH | GPIO_PULL_UP);
	msleep(READ_WAIT_TIME_MS);
	level = gpio_get_level(GPIO_OUTPUT_TEST);
	if (!level)
		return CTS_RC_SUCCESS;
	else
		return CTS_RC_FAILURE;
}

#include "cts_testlist.h"

void cts_task(void)
{
	enum cts_rc result;
	int i;

	uart_flush_output();
	for (i = 0; i < CTS_TEST_ID_COUNT; i++) {
		sync();
		result = tests[i].run();
		CPRINTF("\n%s %d\n", tests[i].name, result);
		uart_flush_output();
	}

	CPRINTS("GPIO test suite finished");
	uart_flush_output();
	while (1) {
		watchdog_reload();
		sleep(1);
	}
}
