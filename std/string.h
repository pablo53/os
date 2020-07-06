/*
 * (C) 2011 Pawel Ryszawa
 * String operations.
 */

#ifndef _STD_STRING_H
#define _STD_STRING_H 1

int strlen(char *);
int strcmp(char *, char *);
int strncmp(char *, char *, unsigned int);
void strcpy(char *, char *);
void strncpy(char *, char *, unsigned int);

#endif

