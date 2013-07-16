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

#include <new.h>
#include <err.h>
#include <squirrel.h> 
#include <dev/class/fb.h>

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

class Surface
{
	private:
	agg::rendering_buffer buf;
	pixfmt pixf;
	base_ren_type rb;
	agg::rgba color;
	glyph_gen glyph;

	public:
	Surface(void *buffer, unsigned width, unsigned height, unsigned scan);
	~Surface(void);

	void setColor(float r, float g, float b, float a);
	void drawRect(float x, float y, float w, float h, float radius, float stroke);
	void fillRect(float x, float y, float w, float h, float radius);

	void getSize(unsigned *w, unsigned *h);

	void drawString(const char *str, int x, int y);

	float getStringWidth(const char *str);
	float getFontHeight(void);
	float getFontBaseLine(void);
};

Surface::Surface(void *buffer, unsigned width, unsigned height, unsigned scan) :
		buf((uint8_t *) buffer, width, height, scan), pixf(buf), rb(pixf), color(), glyph(0)
{
	agg::int8u *font = (agg::int8u *) agg::verdana14;
	glyph.font(font);
}

Surface::~Surface(void)
{
}

void Surface::setColor(float r, float g, float b, float a)
{
	color = agg::rgba(r, g, b, a);
}

void Surface::drawString(const char *str, int x, int y)
{
	agg::renderer_raster_htext_solid<base_ren_type, glyph_gen> rt(rb, glyph);
	rt.color(color);
	rt.render_text(x, y, str, true);
}

float Surface::getStringWidth(const char *str)
{
	return glyph.width(str);
}

float Surface::getFontHeight(void)
{
	return glyph.height();
}

float Surface::getFontBaseLine(void)
{
	return glyph.base_line();
}

void Surface::drawRect(float x, float y, float w, float h, float radius, float stroke)
{
	agg::rect_i r(x, y, x + w, y + h);

	renderer_solid rs(rb);
	rs.color(color);

	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	agg::rounded_rect shape(r.x1, r.y1, r.x2, r.y2, radius);
	agg::conv_stroke<agg::rounded_rect> shape_stroke1(shape);
	shape_stroke1.width(stroke);

	ras.add_path(shape_stroke1);
	agg::render_scanlines(ras, sl, rs);
}

void Surface::fillRect(float x, float y, float w, float h, float radius)
{
	agg::rect_i r(x, y, x + w, y + h);

	renderer_solid rs(rb);
	rs.color(color);

	agg::rasterizer_scanline_aa<> ras;
	agg::scanline_p8 sl;

	agg::rounded_rect shape(r.x1, r.y1, r.x2, r.y2, radius);

	ras.add_path(shape);
	agg::render_scanlines(ras, sl, rs);
}

void Surface::getSize(unsigned *w, unsigned *h)
{
	if (w)
		*w = buf.width();
	
	if (h)
		*h = buf.height();
}

#define LK_SURFACE_TYPE_TAG 0x08080000

#define SETUP_SURFACE(v) \
	Surface *self = NULL; \
	if(SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer*) &self, (SQUserPointer) LK_SURFACE_TYPE_TAG))) \
		return sq_throwerror(v, _SC("invalid type tag")); \
	if(!self) \
		return sq_throwerror(v, _SC("the surface is invalid"));

#define _DECL_SURFACE_FUNC(name, nparams, typecheck) {_SC(#name), _surface_##name, nparams, typecheck}

static SQInteger _surface_releasehook(SQUserPointer p, SQInteger size)
{
	Surface *self = (Surface*) p;
	self->~Surface();

	sq_free(self, sizeof(Surface));
	return 1;
}

static SQInteger _surface_constructor(HSQUIRRELVM v)
{
	struct device *dev = device_get_by_name(fb, fb0);
	struct fb_info info;
	status_t err;

	err = class_fb_get_info(dev, &info);
	if (err != NO_ERROR)
		return sq_throwerror(v, _SC("failed to get fb info"));

	Surface *surface = new (sq_malloc(sizeof(Surface)))
			Surface(info.addr, info.width, info.height, info.line_width);

	if (SQ_FAILED(sq_setinstanceup(v, 1, surface))) {
		surface->~Surface();
		sq_free(surface, sizeof(Surface));

		return sq_throwerror(v, _SC("cannot create surface"));
	}

	sq_setreleasehook(v, 1, _surface_releasehook);
	return 0;
}

static SQInteger _surface_setColor(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	SQFloat r, g, b, a;

	sq_getfloat(v, 2, &r);
	sq_getfloat(v, 3, &g);
	sq_getfloat(v, 4, &b);
	sq_getfloat(v, 5, &a);

	self->setColor(r, g, b, a);

	return 0;
}

static SQInteger _surface_drawString(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	const SQChar *str;
	SQInteger x, y;

	sq_getstring(v, 2, &str);
	sq_getinteger(v, 3, &x);
	sq_getinteger(v, 4, &y);

	self->drawString(str, x, y);

	return 0;
}

static SQInteger _surface_getStringWidth(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	const SQChar *str;
	float width;

	sq_getstring(v, 2, &str);

	width = self->getStringWidth(str);

	sq_pushfloat(v, width);
	return 1;
}

static SQInteger _surface_getFontHeight(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	float height;

	height = self->getFontHeight();

	sq_pushfloat(v, height);
	return 1;
}

static SQInteger _surface_getFontBaseLine(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	float baseline;

	baseline = self->getFontBaseLine();

	sq_pushfloat(v, baseline);
	return 1;
}

static SQInteger _surface_drawRect(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	SQFloat x, y, w, h, r, s;

	sq_getfloat(v, 2, &x);
	sq_getfloat(v, 3, &y);
	sq_getfloat(v, 4, &w);
	sq_getfloat(v, 5, &h);
	sq_getfloat(v, 6, &r);
	sq_getfloat(v, 7, &s);

	self->drawRect(x, y, w, h, r, s);

	return 0;
}

static SQInteger _surface_fillRect(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	SQFloat x, y, w, h, r;

	sq_getfloat(v, 2, &x);
	sq_getfloat(v, 3, &y);
	sq_getfloat(v, 4, &w);
	sq_getfloat(v, 5, &h);
	sq_getfloat(v, 6, &r);

	self->fillRect(x, y, w, h, r);

	return 0;
}

static SQInteger _surface_size(HSQUIRRELVM v)
{
	SETUP_SURFACE(v);
	unsigned w, h;

	self->getSize(&w, &h);

	sq_newtable(v);

	sq_pushstring(v, "width", -1);
	sq_pushinteger(v, w);
	sq_rawset(v, -3);

	sq_pushstring(v, "height", -1);
	sq_pushinteger(v, h);
	sq_rawset(v, -3);

	return 1;
}

static SQInteger _surface__cloned(HSQUIRRELVM v)
{
	return sq_throwerror(v, _SC("this object cannot be cloned"));
}

static SQRegFunction _surface_methods[] = {
	_DECL_SURFACE_FUNC(constructor, 1, _SC("x")),
	_DECL_SURFACE_FUNC(setColor, 5, _SC("xnnnn")),
	_DECL_SURFACE_FUNC(drawString, 4, _SC("xsnn")),
	_DECL_SURFACE_FUNC(getStringWidth, 2, _SC("xs")),
	_DECL_SURFACE_FUNC(getFontHeight, 1, _SC("x")),
	_DECL_SURFACE_FUNC(getFontBaseLine, 1, _SC("x")),
	_DECL_SURFACE_FUNC(drawRect, 7, _SC("xnnnnnn")),
	_DECL_SURFACE_FUNC(fillRect, 6, _SC("xnnnnn")),
	_DECL_SURFACE_FUNC(fillRect, 6, _SC("xnnnnn")),
	_DECL_SURFACE_FUNC(size, 1, _SC("x")),
	_DECL_SURFACE_FUNC(_cloned, 0, NULL),
	{0, 0}
};

static SQInteger _gfx_update(HSQUIRRELVM v)
{
	struct device *dev = device_get_by_name(fb, fb0);

	class_fb_update(dev);

	return 0;
}

static SQInteger _gfx_updateRegion(HSQUIRRELVM v)
{
	struct device *dev = device_get_by_name(fb, fb0);

	SQInteger x, y, w, h;

	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);
	sq_getinteger(v, 4, &w);
	sq_getinteger(v, 5, &h);

	class_fb_update_region(dev, x, y, w, h);

	return 0;
}

#define _DECL_FUNC(name, nparams, pmask) {_SC(#name), _gfx_##name, nparams, pmask}
static SQRegFunction gfxlib_funcs[]={
	_DECL_FUNC(update, 1, _SC(".")),
	_DECL_FUNC(updateRegion, 5, _SC(".nnnn")),
	{0, 0}
};
#undef _DECL_FUNC

static void init_surfaceclass(HSQUIRRELVM v)
{
	sq_pushregistrytable(v);
	sq_pushstring(v, _SC("lk_surface"), -1);

	if (SQ_FAILED(sq_get(v, -2))) {
		sq_pushstring(v, _SC("lk_surface"), -1);

		sq_newclass(v, SQFalse);
		sq_settypetag(v, -1, (SQUserPointer) LK_SURFACE_TYPE_TAG);
		
		SQInteger i = 0;
		while (_surface_methods[i].name != 0) {
			SQRegFunction &f = _surface_methods[i];
			sq_pushstring(v, f.name, -1);
			sq_newclosure(v, f.f, 0);
			sq_setparamscheck(v, f.nparamscheck, f.typemask);
			sq_newslot(v, -3, SQFalse);
			i++;
		}

		sq_newslot(v, -3, SQFalse);

		sq_pushroottable(v);
		sq_pushstring(v, _SC("surface"), -1);
		sq_pushstring(v, _SC("lk_surface"), -1);
		sq_get(v, -4);
		sq_newslot(v, -3, SQFalse);
		sq_pop(v, 1);
	} else {
		sq_pop(v, 1); //result
	}

	sq_pop(v, 1);
}

extern "C"
SQInteger lk_register_gfxlib(HSQUIRRELVM v)
{
	init_surfaceclass(v);

	SQInteger i = 0;
	while (gfxlib_funcs[i].name != 0) {
		sq_pushstring(v, gfxlib_funcs[i].name, -1);
		sq_newclosure(v, gfxlib_funcs[i].f, 0);
		sq_setparamscheck(v, gfxlib_funcs[i].nparamscheck, gfxlib_funcs[i].typemask);
		sq_setnativeclosurename(v, -1, gfxlib_funcs[i].name);
		sq_newslot(v, -3, SQFalse);
		i++;
	}

	return 1;
}

