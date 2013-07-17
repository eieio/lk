#include <math.h>
#include <sys/types.h>

// bytes for +Infinity on a 387
char __infinity[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };

int isnan(double d)
{
	register struct IEEEdp {
		u_int manl : 32;
		u_int manh : 20;
		u_int  exp : 11;
		u_int sign :  1;
	} *p = (struct IEEEdp *)&d;
	
	return(p->exp == 2047 && (p->manh || p->manl));
}

int *__geterrno(void)
{
	static int errno = 0;
	
	return &errno;
}
