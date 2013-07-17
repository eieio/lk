
#ifdef __LINUX__
#define SYMBOL_NAME_STR(X) _##X
#define SYMBOL_NAME(X) _##X
#ifdef __STDC__
#define SYMBOL_NAME_LABEL(X) _##X##:
#else
#define SYMBOL_NAME_LABEL(X) X/**/:
#endif
#else
#define SYMBOL_NAME_STR(X) "_"#X
#ifdef __STDC__
#define SYMBOL_NAME(X) _##X
#define SYMBOL_NAME_LABEL(X) _##X##:
#else
#define SYMBOL_NAME(X) _/**/X
#define SYMBOL_NAME_LABEL(X) _/**/X/**/:
#endif
#endif

#ifdef __LINUX__
# define P2ALIGN(p2)	.align	(1<<(p2))
#else
# define P2ALIGN(p2)	.align	p2
#endif
#ifdef __LINUX__
#define FUNCSYM(x)	.type    x,@function
#else
#define FUNCSYM(x)	/* nothing */
#endif

#define TEXT_ALIGN	4

//#define	ENTRY(x)	FUNCSYM(x); .globl SYMBOL_NAME(x); P2ALIGN(TEXT_ALIGN); SYMBOL_NAME_LABEL(x)
#define ENTRY(x) .global x; .type x,STT_FUNC; x:

#define	ENTRY2(x,y)	.globl SYMBOL_NAME(x); .globl SYMBOL_NAME(y); \
			P2ALIGN(TEXT_ALIGN); LEXT(x) LEXT(y)
#define	ASENTRY(x)	.globl x; P2ALIGN(TEXT_ALIGN); gLB(x)


#define CNAME(x)	SYMBOL_NAME(x)

#define PIC_PROLOGUE
#define PIC_EPILOGUE
#define PIC_PLT(x)      x

#ifndef RCSID
#define RCSID(a)
#endif

