/*
 * (C) 2011 Pawel Ryszawa
 * Math operations.
 */

#ifndef _STD_MATH_H
#define _STD_MATH_H 1

int sgn(int num);
int abs(int num);

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define BETWEEN(x,a,b) ((x) >= a && (x) <= b) /* Be carefull: Do not put non-deterministic function as x! */

#endif

