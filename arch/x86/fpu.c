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

#include <sys/types.h>
#include <debug.h>
#include <malloc.h>
#include <arch/ops.h>
#include <arch/x86.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

static void fpu_init(void);
static void fpu_save_sse(void *buf);
static void fpu_restore_sse(const void *buf);

static thread_t *fpu_thread = NULL; 

void fpu_thread_switch(thread_t *to)
{
	set_in_cr0(X86_CR0_TS); // fault on fpu/mmx/sse
}

void fpu_thread_exit(thread_t *t)
{
	//printf("%s: current_thread=%p (%s), buf=%p\n", __func__, current_thread, current_thread->name, current_thread->arch.fpu_context);

	if (fpu_thread == t)
		fpu_thread = NULL;
	
	free(t->arch.fpu_context);
}

void x86_fpu_exception(struct x86_iframe *frame)
{
	LTRACE_ENTRY;

	clear_in_cr0(X86_CR0_TS); // don't fault on fpu/mmx/sse

	//printf("%s: current_thread=%p (%s), buf=%p\n", __func__, current_thread, current_thread->name, current_thread->arch.fpu_context);
	
	if (fpu_thread == current_thread)
		goto done;
	
	if (fpu_thread) {
		if (!fpu_thread->arch.fpu_context) {
			fpu_thread->arch.fpu_context = memalign(16, 512);
			//printf("%s: fpu_thread=%p (%s), buf=%p\n", __func__, fpu_thread, fpu_thread->name, fpu_thread->arch.fpu_context);
		}

		fpu_save_sse(fpu_thread->arch.fpu_context);
	}

	if (current_thread->arch.fpu_context)
		fpu_restore_sse(current_thread->arch.fpu_context);
	else
		fpu_init();

	fpu_thread = current_thread;

done:
	LTRACE_EXIT;
}

static void fpu_init(void)
{
	//printf("%s: current_thread=%p (%s)\n", __func__, current_thread, current_thread->name);
	 __asm__ __volatile__ ("fninit");
}

static void fpu_save_sse(void *buf)
{
	//printf("%s: buf=%p\n", __func__, buf);
	__asm__ __volatile__ ("fxsave (%0)" :: "r" (buf));
}

static void fpu_restore_sse(const void *buf)
{
	//printf("%s: buf=%p\n", __func__, buf);
	__asm__ __volatile__ ("fxrstor (%0)" :: "r" (buf));
}

