/*
 * Copyright (c) 2013 Corey Tabaka
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

#include <reg.h>
#include <err.h>
#include <debug.h>
#include <trace.h>
#include <assert.h>
#include <malloc.h>
#include <compiler.h>
#include <lib/cbuf.h>
#include <dev/driver.h>
#include <dev/class/i2c.h>
#include <platform/twi.h>
#include <platform/interrupts.h>
#include <kernel/thread.h>

#include <pio/pio.h>
#include <twi/twi.h>
#include <pmc/pmc.h>

#define LOCAL_TRACE 1

static struct device_class twi_device_class = {
	.name = "twi",
};

static status_t drv_twi_init(struct device *dev);
static status_t drv_twi_write(struct device *dev, uint8_t addr, const void *buf, size_t len);
static status_t drv_twi_read(struct device *dev, uint8_t addr, void *buf, size_t len);
static status_t drv_twi_write_reg(struct device *dev, uint8_t addr, uint8_t reg, uint8_t value);
static status_t drv_twi_read_reg(struct device *dev, uint8_t addr, uint8_t reg, void *value);

static struct i2c_ops the_ops = {
	.std = {
		.device_class = &twi_device_class,
		.init = drv_twi_init,
	},

	.write = drv_twi_write,
	.read = drv_twi_read,
	.write_reg = drv_twi_write_reg,
	.read_reg = drv_twi_read_reg,
};

DRIVER_EXPORT(twi, &the_ops.std);

static status_t drv_twi_init(struct device *dev)
{
	LTRACE_ENTRY;

	status_t res = NO_ERROR;
	twi_options_t opt;

	if (!dev)
		return ERR_INVALID_ARGS;

	if (!dev->config)
		return ERR_NOT_CONFIGURED;

	const struct platform_twi_config *config = dev->config;
	dev->state = NULL;

	/* init the peripheral block */
	pmc_enable_periph_clk(config->id);
	pmc_enable_periph_clk(ID_PIOA);

	/* set pin mux config */
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA18);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA17);

	opt.master_clk = 84000000;
	opt.speed = config->speed;
	opt.smbus = 0;

	if (twi_master_init(config->regs, &opt) != TWI_SUCCESS) {
		LTRACEF("Failed to init TWI controller in master mode id=%d\n", config->id);
		res = ERR_GENERIC;
		goto done;
	}

done:
	LTRACE_EXIT;
	return res;
}

void sam3_twi0_irq(void)
{
}

static status_t drv_twi_write(struct device *dev, uint8_t addr, const void *buf, size_t len)
{
	status_t res = NO_ERROR;
	const struct platform_twi_config *config = dev->config;
	if (!config)
		return ERR_NOT_CONFIGURED;

	twi_packet_t packet;

	packet.chip = addr;
	packet.addr_length = 0;
	packet.buffer = (void *) buf;
	packet.length = len;

	if (twi_master_write(config->regs, &packet) != TWI_SUCCESS) {
		LTRACEF("Failed to write to slave id=%d, addr=%02x, buf=%p, len=%zu\n",
				config->id, addr, buf, len);
		res = ERR_GENERIC;
		goto done;
	}

done:
	return res;
}

static status_t drv_twi_read(struct device *dev, uint8_t addr, void *buf, size_t len)
{
	status_t res = NO_ERROR;
	const struct platform_twi_config *config = dev->config;
	if (!config)
		return ERR_NOT_CONFIGURED;

	twi_packet_t packet;

	packet.chip = addr;
	packet.addr_length = 0;
	packet.buffer = buf;
	packet.length = len;

	if (twi_master_read(config->regs, &packet) != TWI_SUCCESS) {
		LTRACEF("Failed to read from slave id=%d, addr=%02x, buf=%p, len=%zu\n",
				config->id, addr, buf, len);
		res = ERR_GENERIC;
		goto done;
	}

done:
	return res;
}

static status_t drv_twi_write_reg(struct device *dev, uint8_t addr, uint8_t reg, uint8_t value)
{
	status_t res = NO_ERROR;
	const struct platform_twi_config *config = dev->config;
	if (!config)
		return ERR_NOT_CONFIGURED;

	twi_packet_t packet;

	packet.chip = addr;
	packet.addr[0] = reg;
	packet.addr_length = 1;
	packet.buffer = &value;
	packet.length = 1;

	if (twi_master_write(config->regs, &packet) != TWI_SUCCESS) {
		LTRACEF("Failed to write to slave id=%d, addr=%02x, reg=%02x, value=%02x\n",
				config->id, addr, reg, value);
		res = ERR_GENERIC;
		goto done;
	}

done:
	return res;
}

static status_t drv_twi_read_reg(struct device *dev, uint8_t addr, uint8_t reg, void *value)
{
	status_t res = NO_ERROR;
	const struct platform_twi_config *config = dev->config;
	if (!config)
		return ERR_NOT_CONFIGURED;

	twi_packet_t packet;

	packet.chip = addr;
	packet.addr[0] = reg;
	packet.addr_length = 1;
	packet.buffer = value;
	packet.length = 1;

	if (twi_master_read(config->regs, &packet) != TWI_SUCCESS) {
		LTRACEF("Failed to read from slave id=%d, addr=%02x, reg=%02x, value=%p\n",
				config->id, addr, reg, value);
		res = ERR_GENERIC;
		goto done;
	}

done:
	return res;
}

