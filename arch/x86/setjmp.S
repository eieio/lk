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

#include <asm.h>

.text
.align 4

FUNCTION(longjmp)
	movl	4(%esp), %eax
	movl	8(%esp), %ebx

	test	%ebx, %ebx
	jnz		0f
	incl	%ebx
0:      
	movl	%ebx, 32(%eax)

	movl	0(%eax), %ebx
	movl	4(%eax), %ecx
	movl	8(%eax), %edx
	movl	12(%eax), %esi
	movl	16(%eax), %edi
	movl	20(%eax), %ebp
	movl	24(%eax), %esp
	pushl	28(%eax)
	movl	32(%eax), %eax
	ret

FUNCTION(setjmp)
	movl	4(%esp), %eax
	movl	%ebx, 0(%eax)
	movl	%ecx, 4(%eax)
	movl	%edx, 8(%eax)
	movl	%esi, 12(%eax)
	movl	%edi, 16(%eax)
	movl	%ebp, 20(%eax)
	popl	%edx
	movl	%esp, 24(%eax)
	movl	%edx, 28(%eax)
	xorl	%eax, %eax
	pushl	%edx
	ret

