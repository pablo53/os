/*
 * 2020 Pawel Ryszawa
 * Math makro definitions.
 */

#ifndef _STD_MATH_DEFS_H
#define _STD_MATH_DEFS_H 1


#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define BETWEEN(x,a,b) ((x) >= a && (x) <= b) /* Be carefull: Do not put non-deterministic function as x! */


#endif

