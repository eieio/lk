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

#include <agg_image_accessors.h>
#include <agg_pixfmt_rgba.h>
#include <agg_rendering_buffer.h>
#include <agg_renderer_base.h>
#include <agg_renderer_scanline.h>
#include <agg_renderer_primitives.h>
#include <agg_renderer_outline_aa.h>
#include <agg_rasterizer_outline.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_rasterizer_outline_aa.h>
#include <agg_scanline_u.h>
#include <agg_scanline_p.h>
#include <agg_scanline_bin.h>
#include <agg_span_allocator.h>
#include <agg_span_gouraud_rgba.h>
#include <agg_span_solid.h>

#include <agg_basics.h>
#include <agg_blur.h>
#include <agg_path_storage.h>
#include <agg_conv_stroke.h>
#include <agg_conv_transform.h>
#include <agg_bounding_rect.h>
//#include <agg_pixfmt_rgb.h>
#include <agg_ellipse.h>
#include <agg_rounded_rect.h>

#include <agg_glyph_raster_bin.h>
#include <agg_renderer_raster_text.h>
#include <agg_embedded_raster_fonts.h>

//#define AGG_GRAY8
//#define AGG_BGR24
//#define AGG_RGB24
#define AGG_BGRA32 
//#define AGG_RGBA32 
//#define AGG_ARGB32 
//#define AGG_ABGR32
//#define AGG_RGB565
//#define AGG_RGB555
#include <pixel_formats.h>

typedef agg::renderer_base<pixfmt> base_ren_type;
typedef agg::span_gouraud_rgba<color_type> span_gen_type;
typedef agg::span_allocator<color_type> span_alloc_type;
typedef agg::renderer_scanline_aa_solid<base_ren_type> renderer_solid;
typedef agg::glyph_raster_bin<agg::rgba8> glyph_gen;

typedef struct image_buffer
{
	agg::rendering_buffer buf;
	pixfmt pixf;
	base_ren_type rb;
	
	image_buffer(void *buffer, int width, int height, int scan) :
		buf((uint8_t *) buffer, width, height, scan), pixf(buf), rb(pixf) {}
	
	void init(void *buffer, int width, int height, int scan)
	{
		buf.attach((uint8_t *) buffer, width, height, scan);
		rb.reset_clipping(true);
	}
} image_buffer;

static image_buffer *ib;
static agg::rgba color;

extern "C"
void _set_color(float r, float g, float b, float a)
{
	color = agg::rgba(r, g, b, a);
}

extern "C"
void _attach_buffer(void *fb, int width, int height, int scan)
{
	//ib.init(fb, width, height, scan);
	if (ib)
		delete ib;
	
	ib = new image_buffer(fb, width, height, scan);
}

extern "C"
void _draw_string(const char *str, int x, int y)
{
	agg::int8u *font = (agg::int8u *) agg::verdana14;

	glyph_gen glyph(0);
	glyph.font(font);
	agg::renderer_raster_htext_solid<base_ren_type, glyph_gen> rt(ib->rb, glyph);
	rt.color(color);
	rt.render_text(x, y, str, true);
}

extern "C"
void _draw_rect(int x, int y, int w, int h, float radius, float stroke)
{
	agg::rect_i r(x, y, x + w, y + h);

	renderer_solid rs(ib->rb);
	rs.color(color);

	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	agg::rounded_rect shape(r.x1, r.y1, r.x2, r.y2, radius);
	agg::conv_stroke<agg::rounded_rect> shape_stroke1(shape);
	shape_stroke1.width(stroke);

	ras.add_path(shape_stroke1);
	agg::render_scanlines(ras, sl, rs);
}

extern "C"
void _fill_rect(int x, int y, int w, int h, float radius)
{
	agg::rect_i r(x, y, x + w, y + h);

	renderer_solid rs(ib->rb);
	rs.color(color);

	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	agg::rounded_rect shape(r.x1, r.y1, r.x2, r.y2, radius);

	ras.add_path(shape);
	agg::render_scanlines(ras, sl, rs);
}

