/*	see copyright notice in squirrel.h */
#ifndef _SQPCHEADER_H_
#define _SQPCHEADER_H_

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#ifdef LK
#include <new.h>
#else
#include <new>
#endif

//squirrel stuff
#include <squirrel.h>
#include "sqobject.h"
#include "sqstate.h"

#endif //_SQPCHEADER_H_
