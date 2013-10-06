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

HSQUIRRELVM init_vm(const SQChar *filename, const SQChar *init)
{
	HSQUIRRELVM v;

	v = sq_open(1024);

	sq_pushroottable(v); //push the root table (were the globals of the script will be stored)

	sqstd_seterrorhandlers(v); //registers the default error handlers
	sq_setprintfunc(v, printfunc, errorfunc); //sets the print function

	sqstd_register_bloblib(v);
	sqstd_register_mathlib(v);
	sqstd_register_stringlib(v);
	sqstd_register_iolib(v);
	//sqstd_register_systemlib(v);
	lk_register_gfxlib(v);
	lk_register_syslib(v);
	lk_register_netlib(v);

	if (SQ_SUCCEEDED(sqstd_dofile(v, filename, SQFalse, SQTrue))) { // also prints syntax errors if any 
		if (init)
			call_vm(v, init);
	} else {
		printf("Failed to open vm script: %s\n", filename);
	}

	sq_pop(v, 1); //pops the root table

	return v;
}

static void sq_init(const struct app_descriptor *app)
{
	sqvm = init_vm(_SC("system/vm/init.nut"), _SC("init"));
}

static void sq_entry(const struct app_descriptor *app, void *args)
{
	call_vm(sqvm, _SC("run"));
	call_vm(sqvm, _SC("close"));

	sq_close(sqvm);
	sqvm = NULL;
}

APP_START(sq)
	.init = sq_init,
	.entry = sq_entry,
APP_END

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

static int sq_cmd(int argc, const cmd_args *argv)
{
	if (argc < 2) {
usage:
		printf("Usage: %s <filename>\n", argv[0].str);
		goto out;
	}
	
	HSQUIRRELVM v = init_vm(argv[1].str, NULL);
	sq_close(v);

out:
	return 0;
}

STATIC_COMMAND_START
{ "sq", "squirrel vm", &sq_cmd },
STATIC_COMMAND_END(sq);

#endif

