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
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <rand.h>
#include <malloc.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <platform.h>
#include <dev/driver.h>
#include <dev/class/spi.h>

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

#define CHAIN_LEN 25

static event_t leds_event = EVENT_INITIAL_VALUE(leds_event, false, 0);
static mutex_t leds_mutex = MUTEX_INITIAL_VALUE(leds_mutex);

static struct led_chain *chain;

static char palette[256 * 3];

static bool red_dir;
static bool green_dir;
static bool blue_dir;

struct led_chain {
	size_t len;
	char elems[0];
};

static inline struct led_chain *chain_alloc(size_t count)
{
	struct led_chain *chain = malloc(sizeof(struct led_chain) + sizeof(char) * count);
	if (chain)
		chain->len = count;
	
	return chain;
}

static void palette_rotate_forward(int component, char *pal)
{
	int i;
	char temp;

	temp = pal[0*3 + component];
	
	for (i=0; i < 256-1; i++)
		pal[(i+0)*3 + component] = pal[(i+1)*3 + component];
	
	pal[(256-1)*3 + component] = temp;
}

static void palette_rotate_backward(int component, char *pal)
{
	int i;
	char temp;

	temp = pal[(256-1)*3 + component];

	for (i=256-2; i >= 0; i--)
		pal[(i+1)*3 + component] = pal[(i+0)*3 + component];
	
	pal[0*3 + component] = temp;
}


static void palette_roll(char *pal)
{
	if (red_dir)
		palette_rotate_forward(0, pal);
	else
		palette_rotate_backward(0, pal);
	
	if (green_dir)
		palette_rotate_forward(1, pal);
	else
		palette_rotate_backward(1, pal);
	
	if (blue_dir)
		palette_rotate_forward(2, pal);
	else
		palette_rotate_backward(2, pal);
}

static void random_invert_direction(void)
{
	switch(rand() % (3 * 30)) {
		case 0:
			red_dir = !red_dir;
			break;

		case 1:
			green_dir = !green_dir;
			break;

		case 2:
			blue_dir = !blue_dir;
			break;
	}
}

static inline void chain_update(struct led_chain *chain)
{
	struct spi_transaction txn;
	uint32_t value;
	unsigned i;

	txn.flags = SPI_WRITE;
	txn.tx_buf = &value;
	txn.rx_buf = NULL;
	txn.len = 3;

	mutex_acquire(&leds_mutex);
	for (i=0; i < chain->len; i++) {
		char *p = palette + chain->elems[i] * 3;
		value = (int) p[0] << 16 | (int) p[1] << 8 | (int) p[2];

		class_spi_transaction(device_get_by_name(spi, spi0), &txn, 1);
	}
	mutex_release(&leds_mutex);
}

#define Q 16
#define K (1 << (Q-1))

typedef int fixed;
typedef long long fixed64;

#define DIVIDE(n, d) ((((fixed64) (n) << Q) + (d)/2) / (d))
#define SCALE(q, s, h) ((((q) >> (Q-(h))) * (s)) >> h)

fixed absf(fixed x)
{
        if (x < 0)
                return -x;
        else
                return x;
}

fixed mult(fixed a, fixed b)
{
        fixed64 temp = (fixed64) a * (fixed64) b;
        temp += K;
        return temp >> Q;
}

static void hsv_to_rgb(fixed h, fixed s, fixed v, char *ro, char *go, char *bo)
{
        fixed r, g, b, c, x, m;

        c = mult(v, s);
		h = h % (1 << Q);
        h = mult(h, 6 << Q);
        x = mult(c, (1 << Q) - absf((h % (2 << Q)) - (1 << Q)));

        h = h >> Q;

        if (0 <= h && h < 1) {
                r = c; g = x; b = 0;
        } else if (1 <= h && h < 2) {
                r = x; g = c; b = 0;
        } else if (2 <= h && h < 3) {
                r = 0; g = c; b = x;
        } else if (3 <= h && h < 4) {
                r = 0; g = x; b = c;
        } else if (4 <= h && h < 5) {
                r = x; g = 0; b = c;
        } else if (5 <= h && h < 6) {
                r = c; g = 0; b = x;
        } else {
                r = 0; g = 0; b = 0;
        }

        m = v - c;

        r += m; g += m; b += m;

		if (ro)
			*ro = mult(r, DIVIDE(255, 1)) >> Q;

		if (go)
        	*go = mult(g, DIVIDE(255, 1)) >> Q;

		if (bo)
        	*bo = mult(b, DIVIDE(255, 1)) >> Q;
}

static int led_cmd(int argc, const cmd_args *argv)
{
	int i;
	struct device *dev = device_get_by_name(spi, spi0);
	struct spi_transaction txn;
	uint32_t value;

	txn.flags = SPI_WRITE;
	txn.tx_buf = &value;
	txn.len = 3;

	if (argc < 2) {
		printf("Not enough arguments:\n");
usage:
		printf("%s: write <int> [<int>, ...]\n", argv[0].str);
		printf("%s: hsv <hue> <sat> <val>\n", argv[0].str);
		goto out;
	}

	if (!strcmp(argv[1].str, "write")) {
		mutex_acquire(&leds_mutex);
		for (i=2; i < argc; i++) {
			value = argv[i].u;
			class_spi_transaction(dev, &txn, 1);
		}
		mutex_release(&leds_mutex);
	} else if (!strcmp(argv[1].str, "update")) {
		if (argc < 3)
			goto usage;

		if (argv[2].b)
			event_signal(&leds_event, true);
		else
			event_unsignal(&leds_event);
	} else if (!strcmp(argv[1].str, "hsv")) {
		if (argc < 6)
			goto usage;

		fixed hue = DIVIDE(argv[2].i, 255);
		fixed sat = DIVIDE(argv[3].i, 255);
		fixed val = DIVIDE(argv[4].i, 255);

		char r, g, b;

		hsv_to_rgb(hue, sat, val, &r, &g, &b);

		mutex_acquire(&leds_mutex);
		for (i=0; i < (int) argv[5].u; i++) {
			value = (int) r << 16 | (int) g << 8 | (int) b;
			class_spi_transaction(dev, &txn, 1);
		}
		mutex_release(&leds_mutex);
	} else {
		goto usage;
	}

out:
	return 0;
}

STATIC_COMMAND_START
{ "leds", "led controls", &led_cmd },
STATIC_COMMAND_END(leds);

static void leds_init(const struct app_descriptor *app)
{
	unsigned i;

#if 0
	for (i=0; i < 128; i++) {
		palette[i*3    ] = (31 + i/2) * 2;
		palette[i*3 + 1] = (31 + i/2) * 2;
		palette[i*3 + 2] = (31 + i/2) * 2;

		palette[(i + 128)*3    ] = (31 + (127 - i)/2)*2;
		palette[(i + 128)*3 + 1] = (31 + (127 - i)/2)*2;
		palette[(i + 128)*3 + 2] = (31 + (127 - i)/2)*2;
	}
#elif 0
	for (i = 0; i < 32; ++i)
	{
		palette[ i        * 3    ] = i * 2;
		palette[(i +  64) * 3    ] = 0;
		palette[(i + 128) * 3    ] = 0;
		palette[(i + 192) * 3    ] = i * 2;

		palette[ i        * 3 + 1] = 0;
		palette[(i +  64) * 3 + 1] = i * 2;
		palette[(i + 128) * 3 + 1] = 0;
		palette[(i + 192) * 3 + 1] = i * 2;

		palette[ i        * 3 + 2] = 0;
		palette[(i +  64) * 3 + 2] = 0;
		palette[(i + 128) * 3 + 2] = i * 2;
		palette[(i + 192) * 3 + 2] = i * 2;
	}

	for (i = 32; i < 64; ++i)
	{
		palette[ i        * 3    ] = (63 - i) * 2;
		palette[(i +  64) * 3    ] = 0;
		palette[(i + 128) * 3    ] = 0;
		palette[(i + 192) * 3    ] = (63 - i) * 2;

		palette[ i        * 3 + 1] = 0;
		palette[(i +  64) * 3 + 1] = (63 - i) * 2;
		palette[(i + 128) * 3 + 1] = 0;
		palette[(i + 192) * 3 + 1] = (63 - i) * 2;

		palette[ i        * 3 + 2] = 0;
		palette[(i +  64) * 3 + 2] = 0;
		palette[(i + 128) * 3 + 2] = (63 - i) * 2;
		palette[(i + 192) * 3 + 2] = (63 - i) * 2;
	}
#elif 0
	for (i = 0; i < 128; ++i)
	{
		palette[i * 3    ] = i/2;
		palette[i * 3 + 1] = i/2;
		palette[i * 3 + 2] = i/2;
	}

	for (i = 128; i < 256; ++i)
	{
		palette[i * 3    ] = (255 - i)/2;
		palette[i * 3 + 1] = (255 - i)/2;
		palette[i * 3 + 2] = (255 - i)/2;
	}
#elif 1
	for (i = 0; i < 64; ++i)
	{
		palette[i        * 3    ] = i;
		palette[i        * 3 + 1] = i;
		palette[i        * 3 + 2] = i;

		palette[(i + 64) * 3    ] = (63 - i);
		palette[(i + 64) * 3 + 1] = (63 - i);
		palette[(i + 64) * 3 + 2] = (63 - i);
	}

	for (i = 128; i < 256; ++i)
	{
		palette[i        * 3    ] = 0;
		palette[i        * 3 + 1] = 0;
		palette[i        * 3 + 2] = 0;
	}
#endif

	chain = chain_alloc(CHAIN_LEN);

#if 0
	for (i=0; i < CHAIN_LEN; i++) {
		chain->elems[i].r = chain->elems[i].g = chain->elems[i].b = 0x0f;
	}
#elif 0
	for (i=0; i < CHAIN_LEN; i++) {
		chain->elems[i] = rand() % 256; //i * 127 / CHAIN_LEN;
	}
#else
	int x, y;
	for (x=0; x < 5; x++)
		for (y=0; y < 5; y++)
			chain->elems[x + y*5] = (x * 5) ^ (y * 5);
#endif

	chain_update(chain);

	event_signal(&leds_event, true);
}

static void leds_entry(const struct app_descriptor *app, void *args)
{
	do {
		event_wait(&leds_event);

#if 0
		int i;

		for (i=0; i < CHAIN_LEN; i++) {
			fixed h = DIVIDE(i, 3 * CHAIN_LEN) + DIVIDE(current_time(), 5000);
			fixed s = DIVIDE(3, 4);
			fixed v = DIVIDE(1, 1);

			hsv_to_rgb(h, s, v, &chain->elems[i].r, &chain->elems[i].g, &chain->elems[i].b);
		}
#endif
		random_invert_direction();
		palette_roll(palette);

		chain_update(chain);

		thread_sleep(1000/90);
	} while (true);
}

APP_START(leds)
	.init = leds_init,
	.entry = leds_entry,
APP_END

#endif

