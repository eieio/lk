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
#include <lib/gfxconsole.h>
#include <dev/display.h>
#include <kernel/thread.h>

#include <vmware/svga_reg.h>
#include <vmware/vm_device_version.h>

#define LOCAL_TRACE 1

static uint16_t index_reg;
static uint16_t value_reg;

static void *fb_ptr;
static size_t fb_len;
static size_t fb_bytes_per_line;

static uint32_t *fifo_ptr;
static size_t fifo_len;

static int capabilities;

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

#define _FIFO_NEXT_CMD (fifo_ptr[SVGA_FIFO_NEXT_CMD])
#define _FIFO_NEXT_CMD_INDEX (fifo_ptr[SVGA_FIFO_NEXT_CMD] / sizeof(uint32_t))
#define _FIFO_STOP (fifo_ptr[SVGA_FIFO_STOP])
#define _FIFO_MAX (fifo_ptr[SVGA_FIFO_MAX])
#define _FIFO_MIN (fifo_ptr[SVGA_FIFO_MIN])

static int vmware_check_version(void);
static uint32_t vmware_read_reg(uint32_t index);
static void vmware_write_reg(uint32_t index, uint32_t value);
static void vmware_wait_for_fb(void);
static void vmware_write_fifo(uint32_t value);

static void vmware_update_region(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

static void vmware_display_init(const struct app_descriptor *app)
{
	pci_location_t loc;
	pci_config_t config;
	uint32_t i;
	int ret;

	ret = pci_find_pci_device(&loc, PCI_DEVICE_ID_VMWARE_SVGA2, PCI_VENDOR_ID_VMWARE, 0);
	if (ret != _PCI_SUCCESSFUL) {
		LTRACEF("Failed to locate display device: 0x%02x\n", ret);
		goto done;
	}

	LTRACEF("Found display device at %02x:%02x\n", loc.bus, loc.dev_fn);

	for (i=0; i < sizeof(config) / sizeof(uint32_t); i++) {
		uint32_t reg = sizeof(uint32_t) * i;

		ret = pci_read_config_word(&loc, reg, ((uint32_t *) &config) + i);
		if (ret != _PCI_SUCCESSFUL) {
			LTRACEF("Failed to read config reg %d: 0x%02x\n", reg, ret);
			goto done;
		}
	}

	for (i=0; i < 6; i++) {
		LTRACEF("BAR[%d]: 0x%08x\n", i, config.base_addresses[i]);
	}

	/* init the register interface */
	uint16_t base_reg = config.base_addresses[0] & ~0x3;
	index_reg = base_reg + SVGA_INDEX_PORT;
	value_reg = base_reg + SVGA_VALUE_PORT;

	ret = vmware_check_version();
	if (ret == (int) SVGA_ID_INVALID)
		goto done;
	
	fb_ptr = (void *) vmware_read_reg(SVGA_REG_FB_START);
	fb_len = vmware_read_reg(SVGA_REG_FB_SIZE);

	fifo_ptr = (uint32_t *) vmware_read_reg(SVGA_REG_MEM_START);
	fifo_len = vmware_read_reg(SVGA_REG_MEM_SIZE) & ~3;

	vmware_write_reg(SVGA_REG_ENABLE, 0);

	LTRACEF("fb_ptr=%p fb_len=%zu fifo_ptr=%p fifo_len=%zu\n", fb_ptr, fb_len, fifo_ptr, fifo_len);

	LTRACEF("Max dimensions: %ux%u %u bpp\n", vmware_read_reg(SVGA_REG_MAX_WIDTH),
			vmware_read_reg(SVGA_REG_MAX_HEIGHT), vmware_read_reg(SVGA_REG_BITS_PER_PIXEL));

	capabilities = vmware_read_reg(SVGA_REG_CAPABILITIES);

	LTRACEF("Capabilities:\n");
	for (i=1; i < 16; i++) {
		int cap = 1 << (i - 1);

		if (capabilities & cap)
			LTRACEF("    %s\n", cap_names[i]);
	}

	vmware_write_reg(SVGA_REG_WIDTH, 800);
	vmware_write_reg(SVGA_REG_HEIGHT, 600);
	vmware_write_reg(SVGA_REG_BITS_PER_PIXEL, 32);

	fb_bytes_per_line = vmware_read_reg(SVGA_REG_BYTES_PER_LINE);

	LTRACEF("Bytes per line: %zu\n", fb_bytes_per_line);
	LTRACEF("FB size: %u\n", vmware_read_reg(SVGA_REG_FB_SIZE));
	LTRACEF("VRAM size: %u\n", vmware_read_reg(SVGA_REG_VRAM_SIZE));
	LTRACEF("FB offset: %u\n", vmware_read_reg(SVGA_REG_FB_OFFSET));

	vmware_write_reg(SVGA_REG_ENABLE, 1);

	_FIFO_MIN = 4 * sizeof(uint32_t);
	_FIFO_MAX = fifo_len;
	_FIFO_NEXT_CMD = 4 * sizeof(uint32_t);
	_FIFO_STOP = 4 * sizeof(uint32_t);

	vmware_write_reg(SVGA_REG_CONFIG_DONE, 1);

	
#if 0
	uint32_t *fb = fb_ptr;
	for (i=0; i < 600; i++) {
#if 0
		int r = i * 0x00 / 600;
		int g = i * 0x20 / 600;
		int b = i * 0xff / 600;
#else
		int r = 0xff;
		int g = 0xff;
		int b = 0xff;
#endif

		for (int j=0; j < fb_bytes_per_line / 4; j++)
			fb[i * fb_bytes_per_line / 4 + j] = ((r & 0xff) << 16) | ((g & 0xff) << 8) | ((b & 0xff) << 0);
	}

	vmware_update_region(0, 0, 800, 600);
#endif

done:
	return;
}

extern void _draw_string(const char *str, int x, int y);
extern void _draw_rect(int x, int y, int w, int h, float radius, float stroke);
extern void _attach_buffer(void *fb, int width, int height, int scan);
extern void _fill_rect(int x, int y, int w, int h, float radius);
extern void _set_color(float r, float g, float b, float a);

#include <math.h>

static void vmware_display_entry(const struct app_descriptor *app, void *args)
{
	_attach_buffer(fb_ptr, 800, 600, fb_bytes_per_line);

	_set_color(0.6, 0.6, 0.6, 1.0);
	_fill_rect(0, 0, 800, 600, 0.0);

	_set_color(0.0, 0.0, 0.0, 0.4);
	_draw_string("Test string", 20+1, 20+1);
	_draw_string("Welcome to LK", 20+1, 40+1);

	_set_color(1.0, 0.0, 0.0, 1.0);
	_draw_string("Test string", 20, 20);
	_draw_string("Welcome to LK", 20, 40);

	_set_color(0.8, 0.8, 0.8, 0.4);
	_fill_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0);

	_set_color(0.2, 0.2, 0.2, 1.0);
	_draw_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0, 1.5);

	vmware_update_region(0, 0, 800, 600);

	float a = 0.0;
	while (true) {
		thread_sleep(1000 / 24);

		_set_color(0.6, 0.6, 0.6, 1.0);
		_fill_rect(800 / 4 - 5, 600 / 8 - 5, 800 / 2 + 10, 600 / 2 + 10, 0.0);

		_set_color(0.8, 0.8, 0.8, a);
		_fill_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0);

		_set_color(0.2, 0.2, 0.2, 1.0);
		_draw_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0, 1.5);

		vmware_update_region(800 / 4 - 5, 600 / 8 - 5, 800 / 2 + 10, 600 / 2 + 10);

		a = fmod(a + 1.0 / 24.0, 1.0);
	}
}

static int vmware_check_version(void)
{
	int svga_id;
	
	vmware_write_reg(SVGA_REG_ID, SVGA_ID_2);
	svga_id = vmware_read_reg(SVGA_REG_ID);
	if (svga_id == SVGA_ID_2) {
		LTRACEF("Display device supports SVGA_ID_2\n");
		return SVGA_ID_2;
	}
	
	vmware_write_reg(SVGA_REG_ID, SVGA_ID_1);
	svga_id = vmware_read_reg(SVGA_REG_ID);
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

static uint32_t vmware_read_reg(uint32_t index)
{
	outpd(index_reg, index);
	return inpd(value_reg);
}

static void vmware_write_reg(uint32_t index, uint32_t value)
{
	outpd(index_reg, index);
	outpd(value_reg, value);
}

static void vmware_wait_for_fb(void)
{
	vmware_write_reg(SVGA_REG_SYNC, 1);
	while (vmware_read_reg(SVGA_REG_BUSY));
}

static void vmware_write_fifo(uint32_t value)
{
	if ((_FIFO_NEXT_CMD + sizeof(uint32_t) == _FIFO_STOP)
			|| (_FIFO_NEXT_CMD == _FIFO_MAX - sizeof(uint32_t) && _FIFO_STOP == _FIFO_MIN)) {
		vmware_wait_for_fb();
	}

	fifo_ptr[_FIFO_NEXT_CMD_INDEX] = value;

	if (_FIFO_NEXT_CMD == _FIFO_MAX - sizeof(uint32_t))
		_FIFO_NEXT_CMD = _FIFO_MIN;
	else
		_FIFO_NEXT_CMD += sizeof(uint32_t);
}

static void vmware_update_region(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	vmware_write_fifo(SVGA_CMD_UPDATE);
	vmware_write_fifo(x);
	vmware_write_fifo(y);
	vmware_write_fifo(w);
	vmware_write_fifo(h);
}

static void _flush(uint starty, uint endy)
{
	vmware_update_region(0, 0, 800, 600);
}

void display_get_info(struct display_info *info)
{
	info->framebuffer = fb_ptr;
	info->format = GFX_FORMAT_RGB_x888;
	info->width = 800;
	info->height = 600;
	info->stride = fb_bytes_per_line / 4;
	info->flush = _flush;
}

APP_START(display)
	.init = vmware_display_init,
	.entry = vmware_display_entry,
APP_END

