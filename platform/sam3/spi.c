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
#include <assert.h>
#include <malloc.h>
#include <lib/cbuf.h>
#include <dev/driver.h>
#include <dev/class/spi.h>
#include <platform/spi.h>
#include <platform/interrupts.h>

#include <pio/pio.h>
#include <spi/spi.h>
#include <pmc/pmc.h>

static struct device_class spi_device_class = {
	.name = "spi",
};

struct spi_driver_state {
	struct cbuf rx_buf;
	struct cbuf tx_buf;
};

static status_t drv_spi_init(struct device *dev);
static ssize_t drv_spi_transaction(struct device *dev, struct spi_transaction *txn, size_t count);

static struct spi_ops the_ops = {
	.std = {
		.device_class = &spi_device_class,
		.init = drv_spi_init,
	},
	.transaction = drv_spi_transaction,
};

DRIVER_EXPORT(spi, &the_ops.std);

static status_t drv_spi_init(struct device *dev)
{
	status_t res = NO_ERROR;

	if (!dev)
		return ERR_INVALID_ARGS;

	if (!dev->config)
		return ERR_NOT_CONFIGURED;

	const struct platform_spi_config *config = dev->config;

	struct spi_driver_state *state = malloc(sizeof(struct spi_driver_state));
	if (!state) {
		res = ERR_NO_MEMORY;
		goto done;
	}

	dev->state = state;

	/* init the peripheral block */
	pmc_enable_periph_clk(config->id);
	spi_disable(config->regs);
	spi_reset(config->regs);
	spi_set_lastxfer(config->regs);
	spi_set_master_mode(config->regs);
	spi_disable_mode_fault_detect(config->regs);
	spi_set_peripheral_chip_select_value(config->regs, config->cs);
	spi_set_clock_polarity(config->regs, config->cs, config->clk_pol);
	spi_set_clock_phase(config->regs, config->cs, config->clk_phase);
	spi_set_bits_per_transfer(config->regs, config->cs, SPI_CSR_BITS_8_BIT);
	spi_set_baudrate_div(config->regs, config->cs, (84000000 / config->clk_rate));
	//spi_enable_interrupt(config->regs, SPI_IER_RDRF);
	spi_enable(config->regs);

done:
	return res;
}

void sam3_spi0_irq(void)
{
}

static ssize_t drv_spi_transaction(struct device *dev, struct spi_transaction *txn, size_t count)
{

}

