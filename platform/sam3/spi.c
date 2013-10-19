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
#include <lib/cbuf.h>
#include <dev/driver.h>
#include <dev/class/spi.h>
#include <platform/spi.h>
#include <platform/interrupts.h>

#include <pio/pio.h>
#include <spi/spi.h>
#include <pmc/pmc.h>

#define LOCAL_TRACE 0

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

	LTRACE_ENTRY;

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

	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA25);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA26);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA27);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA28);

	spi_disable(config->regs);
	spi_reset(config->regs);
	spi_set_lastxfer(config->regs);
	spi_set_master_mode(config->regs);
	spi_disable_mode_fault_detect(config->regs);
	
	if (config->ps)
		spi_set_variable_peripheral_select(config->regs);
	else
		spi_set_fixed_peripheral_select(config->regs);

	spi_set_peripheral_chip_select_value(config->regs, config->pcs);

	if (!config->ps) {
		spi_set_clock_polarity(config->regs, 0, config->clk_pol);
		spi_set_clock_phase(config->regs, 0, config->clk_phase);
		spi_set_bits_per_transfer(config->regs, 0, SPI_CSR_BITS_8_BIT);
		spi_set_baudrate_div(config->regs, 0, (84000000 / config->clk_rate));
	} else {
		// TODO: configure each chip select settings
	}

	//spi_enable_interrupt(config->regs, SPI_IER_RDRF);
	spi_enable(config->regs);

done:
	LTRACE_EXIT;
	return res;
}

void sam3_spi0_irq(void)
{
}

static ssize_t drv_spi_transaction(struct device *dev, struct spi_transaction *txn, size_t count)
{
	size_t i, j, total = 0;
	uint8_t pcs;
	uint16_t data;

	LTRACE_ENTRY;

	if (!dev)
		return ERR_INVALID_ARGS;

	if (!dev->config)
		return ERR_NOT_CONFIGURED;

	const struct platform_spi_config *config = dev->config;

	for (i=0; i < count; i++) {
		if (txn[i].flags & SPI_CS_ASSERT) {
			// TODO
		}

		// validate flags and buffers for this part of the transaction
		if ((txn[i].flags & SPI_WRITE) && (txn[i].tx_buf == NULL))
			return ERR_INVALID_ARGS;

		if ((txn[i].flags & SPI_READ) && (txn[i].rx_buf == NULL))
			return ERR_INVALID_ARGS;

		for (j=0; j < txn[i].len; j++) {
			if (txn[i].flags & SPI_WRITE)
				spi_write(config->regs, ((uint8_t *) txn[i].tx_buf)[j], 0, 0);

			while ((spi_read_status(config->regs) & SPI_SR_RDRF) == 0);

			if (txn[i].flags & SPI_READ) {
				spi_read(config->regs, &data, &pcs);
				((uint8_t *) txn[i].rx_buf)[j] = data;
			}
		}

		total += txn[i].len;

		if (txn[i].flags & SPI_CS_DEASSERT) {
			// TODO
		}
	}
	
	LTRACE_EXIT;
	return total;
}

