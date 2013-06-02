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
#include <lib/gfxconsole.h>
#include <dev/display.h>
#include <kernel/thread.h>
#include <dev/driver.h>
#include <dev/class/fb.h>
#include <err.h>

#define LOCAL_TRACE 1

static struct fb_info info;

static void vmware_display_init(const struct app_descriptor *app)
{
	struct device *dev = device_get_by_name(fb, fb0);
	status_t err;
	
	err = class_fb_get_info(dev, &info);
	if (err != NO_ERROR) {
		LTRACEF("Error while getting fb info: %d\n", err);
		return;
	}
}

extern void _draw_string(const char *str, int x, int y);
extern void _draw_rect(int x, int y, int w, int h, float radius, float stroke);
extern void _attach_buffer(void *fb, int width, int height, int scan);
extern void _fill_rect(int x, int y, int w, int h, float radius);
extern void _set_color(float r, float g, float b, float a);

#include <math.h>

static void vmware_display_entry(const struct app_descriptor *app, void *args)
{
	struct device *dev = device_get_by_name(fb, fb0);

	_attach_buffer(info.addr, 800, 600, info.line_width);

	_set_color(0.6, 0.6, 0.6, 1.0);
	_fill_rect(0, 0, 800, 600, 0.0);

#if 0
	_set_color(0.0, 0.0, 0.0, 0.4);
	_draw_string("Test string", 20+1, 20+1);
	_draw_string("Welcome to LK", 20+1, 40+1);
#endif

	_set_color(0.2, 0.2, 0.2, 1.0);
	_draw_string("Test string", 20, 20);
	_draw_string("Welcome to LK", 20, 40);

	_set_color(0.8, 0.8, 0.8, 0.4);
	_fill_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0);

	_set_color(0.2, 0.2, 0.2, 1.0);
	_draw_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0, 1.5);

	class_fb_update_region(dev, 0, 0, 800, 600);

	float a = 0.0;
	while (true) {
		thread_sleep(1000 / 24);

		_set_color(0.6, 0.6, 0.6, 1.0);
		_fill_rect(800 / 4 - 5, 600 / 8 - 5, 800 / 2 + 10, 600 / 2 + 10, 0.0);

		_set_color(0.8, 0.8, 0.8, a);
		_fill_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0);

		_set_color(0.2, 0.2, 0.2, 1.0);
		_draw_rect(800 / 4, 600 / 8, 800 / 2, 600 / 2, 1.0, 1.5);

		class_fb_update_region(dev, 800 / 4 - 5, 600 / 8 - 5, 800 / 2 + 10, 600 / 2 + 10);

		a = fmod(a + 1.0 / 24.0, 1.0);
	}
}

static void _flush(uint starty, uint endy)
{
	struct device *dev = device_get_by_name(fb, fb0);
	class_fb_update(dev);
}

void display_get_info(struct display_info *info)
{
	struct device *dev = device_get_by_name(fb, fb0);
	struct fb_info fb;

	class_fb_get_info(dev, &fb);

	info->framebuffer = fb.addr;
	info->format = GFX_FORMAT_RGB_x888;
	info->width = fb.width;
	info->height = fb.height;
	info->stride = fb.line_width / 4;
	info->flush = _flush;
}

APP_START(display)
	.init = vmware_display_init,
	.entry = vmware_display_entry,
APP_END

