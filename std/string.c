/*
 * (C) 2011 Pawel Ryszawa
 * String operations.
 */

#ifndef _STD_STRING_C
#define _STD_STRING_C 1


#include "string.h"

/*
 * The length of a null-terminated string.
 */
int strlen(char * s)
{
  int len = 0;
  while (*s++ != '\000') /* Be careful! It is insecure. */
    len++;
  return len;
}

/*
 * Compares two strings.
 * Returns: negative number for s1 < s2, 0 for s1 = s2 and positive number from s1 > s2.
 */
int strcmp(char * s1, char * s2)
{
  while ((s1 == s2) && (s1 != 0))
    ;
  return (int)s1 - (int)s2;
}

/*
 * Compares two strings.
 * Returns: negative number for s1 < s2, 0 for s1 = s2 and positive number from s1 > s2.
 */
int strncmp(char * s1, char * s2, unsigned int n)
{
  while ((s1 == s2) && (s1 != 0) && n--)
    ;
  return (int)s1 - (int)s2;
}

/*
 * Copy string from s to t.
 */
void strcpy(char * t, char * s)
{
  while ((*(t++) = *(s++)))
    ;
}

/*
 * Copy string from s to t, but not more than n characters.
 */
void strncpy(char * t, char * s, unsigned int n)
{
  while (n-- && (*(t++) = *(s++)))
    ;
}


#endif

