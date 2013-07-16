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

#include <err.h>
#include <squirrel.h> 
#include <platform.h>
#include <platform/keyboard.h>
#include <kernel/thread.h>

static SQInteger _sys_sleep(HSQUIRRELVM v)
{
	SQInteger ms;

	sq_getinteger(v, 2, &ms);

	thread_sleep(ms);

	return 0;
}

static SQInteger _sys_getCurrentTime(HSQUIRRELVM v)
{
	lk_time_t time = current_time();

	sq_pushinteger(v, time);
	return 1;
}

static SQInteger _sys_getKey(HSQUIRRELVM v)
{
	SQInteger nargs;
	SQBool wait = false;
	int ret;
	char c;

	nargs = sq_gettop(v);
	
	if (nargs == 2)
		sq_getbool(v, 2, &wait);
	
	ret = platform_read_key(&c, wait);

	if (ret)
		sq_pushstring(v, &c, 1);
	else
		sq_pushnull(v);

	return 1;
}

#define _DECL_FUNC(name, nparams, pmask) {_SC(#name), _sys_##name, nparams, pmask}
static SQRegFunction syslib_funcs[] = {
	_DECL_FUNC(sleep, 2, _SC(".n")),
	_DECL_FUNC(getCurrentTime, 1, _SC(".")),
	_DECL_FUNC(getKey, -1, _SC(".b")),
	{0, 0, 0, 0}
};
#undef _DECL_FUNC

SQInteger lk_register_syslib(HSQUIRRELVM v)
{
	SQInteger i = 0;
	while (syslib_funcs[i].name != 0) {
		sq_pushstring(v, syslib_funcs[i].name, -1);
		sq_newclosure(v, syslib_funcs[i].f, 0);
		sq_setparamscheck(v, syslib_funcs[i].nparamscheck, syslib_funcs[i].typemask);
		sq_setnativeclosurename(v, -1, syslib_funcs[i].name);
		sq_newslot(v, -3, SQFalse);
		i++;
	}

	return 1;
}

