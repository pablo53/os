/*
 * (C) 2011 Pawel Ryszawa
 */
 
#ifndef _MAIN_LTERM_H
#define _MAIN_LTERM_H 1


#include "../globals.h"

#include "../asm/types.h"

#define TMAX_X 80
#define TMAX_Y 25

#define TTAB_SIZE 5

void tscroll1(void);
void tputck(char);
void tprintk(char *);
void tprintfk(char *, ...);
void tmvcur(int, int);
void tprinthexb(u8);
void tprinthexw(u16);
void tprinthexl(u32);
void tprintdec(int);
int tprintdeca(int, int);


#endif
