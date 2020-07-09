/*
 * (C) 2011 Pawel Ryszawa
 * Math operations.
 */

#ifndef _STD_MATH_C
#define _STD_MATH_C 1


#include "math.h"

int sgn(int num)
{
  return (num > 0) ? 1 : (num < 0) ? -1 : 0;
}

int abs(int num)
{
  return (num >= 0) ? num : -num;
}


#endif

