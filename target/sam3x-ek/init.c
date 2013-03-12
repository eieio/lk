/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <err.h>
#include <debug.h>
#include <target.h>
#include <compiler.h>
#include <dev/driver.h>
#include <platform/spi.h>
#include <platform/twi.h>

static const struct platform_spi_config spi0_config = {
	.regs = SPI0,
	.id = ID_SPI0,
	.clk_rate = 1000000,
};
DEVICE_INSTANCE(spi, spi0, &spi0_config);

static const struct platform_twi_config twi0_config = {
	.regs = TWI0,
	.id = ID_TWI0,
	.speed = 100000,
};
DEVICE_INSTANCE(twi, twi0, &twi0_config);

void target_early_init(void)
{
}

void target_init(void)
{
	TRACE_ENTRY;

	device_init_all();

	TRACE_EXIT;
}

