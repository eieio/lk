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
#include <app.h>
#include <debug.h>
#include <string.h>
#include <lib/console.h>
#include <dev/pci.h>
#include <arch/x86.h>
#include <dev/driver.h>
#include <dev/class/fb.h>
#include <err.h>
#include <malloc.h>
#include <platform/vmware_svga.h>

#include <vmware/svga_reg.h>
#include <vmware/vm_device_version.h>

#define LOCAL_TRACE 1

struct vmware_svga_state {
	uint16_t index_reg;
	uint16_t value_reg;

	void *fb_ptr;
	size_t fb_len;
	size_t fb_bytes_per_line;

	uint32_t *fifo_ptr;
	size_t fifo_len;

	int capabilities;

	size_t width;
	size_t height;
	size_t bpp;
};

static const char *cap_names[] = {
	"NONE",
	"RECT_FILL",
	"RECT_COPY",
	"RECT_PAT_FILL",
	"LEGACY_OFFSCREEN",
	"RASTER_OP",
	"CURSOR",
	"CURSOR_BYPASS",
	"CURSOR_BYPASS_2",
	"8BIT_EMULATION",
	"ALPHA_CURSOR",
	"GLYPH",
	"GLYPH_CLIPPING",
	"OFFSCREEN_1",
	"ALPHA_BLEND",
};

#define _FIFO_NEXT_CMD (state->fifo_ptr[SVGA_FIFO_NEXT_CMD])
#define _FIFO_NEXT_CMD_INDEX (state->fifo_ptr[SVGA_FIFO_NEXT_CMD] / sizeof(uint32_t))
#define _FIFO_STOP (state->fifo_ptr[SVGA_FIFO_STOP])
#define _FIFO_MAX (state->fifo_ptr[SVGA_FIFO_MAX])
#define _FIFO_MIN (state->fifo_ptr[SVGA_FIFO_MIN])

static status_t vmware_svga_init(struct device *dev);

static int vmware_check_version(struct device *dev);
static uint32_t vmware_read_reg(struct device *dev, uint32_t index);
static void vmware_write_reg(struct device *dev, uint32_t index, uint32_t value);
static void vmware_wait_for_fb(struct device *dev);
static void vmware_write_fifo(struct device *dev, uint32_t value);

static status_t vmware_set_mode(struct device *dev, size_t width, size_t height, size_t bpp);
static status_t vmware_get_info(struct device *dev, struct fb_info *info);
static status_t vmware_update(struct device *dev);
static status_t vmware_update_region(struct device *dev, size_t x, size_t y, size_t w, size_t h);

static struct fb_ops the_ops = {
	.std = {
		.init = vmware_svga_init,
	},

	.set_mode = vmware_set_mode,
	.get_info = vmware_get_info,
	.update = vmware_update,
	.update_region = vmware_update_region,
};

DRIVER_EXPORT(fb, &the_ops.std);

static status_t vmware_svga_init(struct device *dev)
{
	pci_location_t loc;
	pci_config_t cfg;
	status_t res = NO_ERROR;
	uint32_t i;
	int ret;

	if (!dev)
		return ERR_INVALID_ARGS;
	
	if (!dev->config)
		return ERR_NOT_CONFIGURED;
	
	const struct platform_vmware_svga_config *config = dev->config;

	ret = pci_find_pci_device(&loc, PCI_DEVICE_ID_VMWARE_SVGA2, PCI_VENDOR_ID_VMWARE, 0);
	if (ret != _PCI_SUCCESSFUL) {
		LTRACEF("Failed to locate display device: 0x%02x\n", ret);
		res = ERR_NOT_FOUND;
		goto done;
	}

	LTRACEF("Found display device at %02x:%02x\n", loc.bus, loc.dev_fn);

	for (i=0; i < sizeof(cfg) / sizeof(uint32_t); i++) {
		uint32_t reg = sizeof(uint32_t) * i;

		ret = pci_read_config_word(&loc, reg, ((uint32_t *) &cfg) + i);
		if (ret != _PCI_SUCCESSFUL) {
			LTRACEF("Failed to read config reg %d: 0x%02x\n", reg, ret);
			res = ERR_NOT_CONFIGURED;
			goto done;
		}
	}

	for (i=0; i < 6; i++) {
		LTRACEF("BAR[%d]: 0x%08x\n", i, cfg.base_addresses[i]);
	}

	struct vmware_svga_state *state = malloc(sizeof(struct vmware_svga_state));
	if (!state) {
		res = ERR_NO_MEMORY;
		goto done;
	}
	dev->state = state;

	/* init the register interface */
	uint16_t base_reg = cfg.base_addresses[0] & ~0x3;
	state->index_reg = base_reg + SVGA_INDEX_PORT;
	state->value_reg = base_reg + SVGA_VALUE_PORT;

	ret = vmware_check_version(dev);
	if (ret == (int) SVGA_ID_INVALID) {
		res = ERR_NOT_FOUND;
		goto done;
	}
	
	state->fb_ptr = (void *) vmware_read_reg(dev, SVGA_REG_FB_START);
	state->fb_len = vmware_read_reg(dev, SVGA_REG_FB_SIZE);

	state->fifo_ptr = (uint32_t *) vmware_read_reg(dev, SVGA_REG_MEM_START);
	state->fifo_len = vmware_read_reg(dev, SVGA_REG_MEM_SIZE) & ~3;

	vmware_write_reg(dev, SVGA_REG_ENABLE, 0);

	LTRACEF("fb_ptr=%p fb_len=%zu fifo_ptr=%p fifo_len=%zu\n",
			state->fb_ptr, state->fb_len, state->fifo_ptr, state->fifo_len);

	LTRACEF("Max dimensions: %ux%u %u bpp\n", vmware_read_reg(dev, SVGA_REG_MAX_WIDTH),
			vmware_read_reg(dev, SVGA_REG_MAX_HEIGHT), vmware_read_reg(dev, SVGA_REG_BITS_PER_PIXEL));

	state->capabilities = vmware_read_reg(dev, SVGA_REG_CAPABILITIES);

	LTRACEF("Capabilities:\n");
	for (i=1; i < 16; i++) {
		int cap = 1 << (i - 1);

		if (state->capabilities & cap)
			LTRACEF("    %s\n", cap_names[i]);
	}

	vmware_write_reg(dev, SVGA_REG_ENABLE, 1);

	vmware_set_mode(dev, config->default_mode.width, config->default_mode.height,
			config->default_mode.bpp);

	_FIFO_MIN = 4 * sizeof(uint32_t);
	_FIFO_MAX = state->fifo_len;
	_FIFO_NEXT_CMD = 4 * sizeof(uint32_t);
	_FIFO_STOP = 4 * sizeof(uint32_t);

	vmware_write_reg(dev, SVGA_REG_CONFIG_DONE, 1);
	
done:
	return res;
}

static status_t vmware_set_mode(struct device *dev, size_t width, size_t height, size_t bpp)
{
	struct vmware_svga_state *state = dev->state;

	vmware_write_reg(dev, SVGA_REG_ENABLE, 0);

	vmware_write_reg(dev, SVGA_REG_WIDTH, width);
	vmware_write_reg(dev, SVGA_REG_HEIGHT, height);
	vmware_write_reg(dev, SVGA_REG_BITS_PER_PIXEL, bpp);

	state->width = width;
	state->height = height;
	state->bpp = bpp;

	state->fb_bytes_per_line = vmware_read_reg(dev, SVGA_REG_BYTES_PER_LINE);

	LTRACEF("Bytes per line: %zu\n", state->fb_bytes_per_line);
	LTRACEF("FB size: %u\n", vmware_read_reg(dev, SVGA_REG_FB_SIZE));
	LTRACEF("VRAM size: %u\n", vmware_read_reg(dev, SVGA_REG_VRAM_SIZE));
	LTRACEF("FB offset: %u\n", vmware_read_reg(dev, SVGA_REG_FB_OFFSET));

	vmware_write_reg(dev, SVGA_REG_ENABLE, 1);

	return NO_ERROR;
}

static int vmware_check_version(struct device *dev)
{
	int svga_id;
	
	vmware_write_reg(dev, SVGA_REG_ID, SVGA_ID_2);
	svga_id = vmware_read_reg(dev, SVGA_REG_ID);
	if (svga_id == SVGA_ID_2) {
		LTRACEF("Display device supports SVGA_ID_2\n");
		return SVGA_ID_2;
	}
	
	vmware_write_reg(dev, SVGA_REG_ID, SVGA_ID_1);
	svga_id = vmware_read_reg(dev, SVGA_REG_ID);
	if (svga_id == SVGA_ID_1) {
		LTRACEF("Display device supports SVGA_ID_1\n");
		return SVGA_ID_1;
	}
	
	if (svga_id == SVGA_ID_0) {
		LTRACEF("Display device supports SVGA_ID_0\n");
		return SVGA_ID_0;
	}
	
	LTRACEF("No supported SVGA_ID");
	return SVGA_ID_INVALID;
}

static uint32_t vmware_read_reg(struct device *dev, uint32_t index)
{
	struct vmware_svga_state *state = dev->state;

	outpd(state->index_reg, index);
	return inpd(state->value_reg);
}

static void vmware_write_reg(struct device *dev, uint32_t index, uint32_t value)
{
	struct vmware_svga_state *state = dev->state;

	outpd(state->index_reg, index);
	outpd(state->value_reg, value);
}

static void vmware_wait_for_fb(struct device *dev)
{
	vmware_write_reg(dev, SVGA_REG_SYNC, 1);
	while (vmware_read_reg(dev, SVGA_REG_BUSY));
}

static void vmware_write_fifo(struct device *dev, uint32_t value)
{
	struct vmware_svga_state *state = dev->state;

	if ((_FIFO_NEXT_CMD + sizeof(uint32_t) == _FIFO_STOP)
			|| (_FIFO_NEXT_CMD == _FIFO_MAX - sizeof(uint32_t) && _FIFO_STOP == _FIFO_MIN)) {
		vmware_wait_for_fb(dev);
	}

	state->fifo_ptr[_FIFO_NEXT_CMD_INDEX] = value;

	if (_FIFO_NEXT_CMD == _FIFO_MAX - sizeof(uint32_t))
		_FIFO_NEXT_CMD = _FIFO_MIN;
	else
		_FIFO_NEXT_CMD += sizeof(uint32_t);
}

static status_t vmware_get_info(struct device *dev, struct fb_info *info)
{
	struct vmware_svga_state *state = dev->state;

	if (!info)
		return ERR_INVALID_ARGS;
	
	info->addr = state->fb_ptr;
	info->width = state->width;
	info->height = state->height;
	info->bpp = state->bpp;
	info->line_width = state->fb_bytes_per_line;
	
	return NO_ERROR;
}

static status_t vmware_update(struct device *dev)
{
	struct vmware_svga_state *state = dev->state;

	return vmware_update_region(dev, 0, 0, state->width, state->height);
}

static status_t vmware_update_region(struct device *dev, size_t x, size_t y, size_t w, size_t h)
{
	vmware_write_fifo(dev, SVGA_CMD_UPDATE);
	vmware_write_fifo(dev, x);
	vmware_write_fifo(dev, y);
	vmware_write_fifo(dev, w);
	vmware_write_fifo(dev, h);

	return NO_ERROR;
}

