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
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <compiler.h>
#include <platform.h>

#include <squirrel.h> 
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>	
#include <sqstdstring.h>
#include <sqstdaux.h>

HSQUIRRELVM sqvm; 
SQInteger lk_register_gfxlib(HSQUIRRELVM v);
SQInteger lk_register_syslib(HSQUIRRELVM v);
SQInteger lk_register_netlib(HSQUIRRELVM v);

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

#endif

static void printfunc(HSQUIRRELVM v, const SQChar *s, ...)
{
	va_list vl;

	va_start(vl, s);
	_dvprintf(s, vl);
	va_end(vl);
}

static void errorfunc(HSQUIRRELVM v, const SQChar *s, ...)
{
	va_list vl;

	va_start(vl, s);
	_dvprintf(s, vl);
	va_end(vl);
}

void call_vm(HSQUIRRELVM v, const SQChar *s)
{
	SQInteger top = sq_gettop(v); //saves the stack size before the call

	sq_pushroottable(v); //pushes the global table
	sq_pushstring(v, _SC("raise"), -1);

	if(SQ_SUCCEEDED(sq_get(v, -2))) { //gets the field 'raise' from the global table
		sq_pushroottable(v); //push the 'this' (in this case is the global table)
		sq_pushstring(v, s, -1);

		//printf("--- Raising event: %s\n", s);
		sq_call(v, 2, SQFalse, SQTrue); //calls the function 
	} else {
		printf("Failed to call raise\n");
	}

	sq_settop(v, top); //restores the original stack size
}

static void sq_init(const struct app_descriptor *app)
{
	// early script engine init here
	sqvm = sq_open(1024);

	sq_pushroottable(sqvm); //push the root table (were the globals of the script will be stored)

	sqstd_seterrorhandlers(sqvm); //registers the default error handlers
	sq_setprintfunc(sqvm, printfunc, errorfunc); //sets the print function

	sqstd_register_bloblib(sqvm);
	sqstd_register_mathlib(sqvm);
	sqstd_register_stringlib(sqvm);
	sqstd_register_iolib(sqvm);
	//sqstd_register_systemlib(sqvm);
	lk_register_gfxlib(sqvm);
	lk_register_syslib(sqvm);
	lk_register_netlib(sqvm);

	if (SQ_SUCCEEDED(sqstd_dofile(sqvm, _SC("system/vm/init.nut"), SQFalse, SQTrue))) { // also prints syntax errors if any 
		call_vm(sqvm, _SC("init"));
	} else {
		printf("Failed to open vm script\n");
	}

	sq_pop(sqvm, 1); //pops the root table
}

static void sq_entry(const struct app_descriptor *app, void *args)
{
	call_vm(sqvm, _SC("run"));
}

APP_START(sq)
	.init = sq_init,
	.entry = sq_entry,
APP_END

