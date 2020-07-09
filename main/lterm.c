/*
 * (C) 2011 Pawel Ryszawa
 *
 * It is recommended to include this file directly by #inlcude directive.
 * This file is intended for use during initialization routine by /main.c only.
 */


#ifndef _MAIN_LTERM_C
#define _MAIN_LTERM_C 1


#include "../globals.h"
#include "lterm.h"

#include "../std/string.h"
#include "../std/math.h"
#include "system.h"

#include <stdarg.h>

/* Graphic card ports: */
#define VGA_REG_ADDR 0x3c4
#define VGA_REG_DATA 0x3c5

/* Cursor coordinates in the local terminal: */
static volatile int tcurx = 0;
static volatile int tcury = 0;

/*
 * Feed 1 line.
 * Be careful: interrupts MUST be masked (disabled) or using the
 * same segments (so as not to "loose" their base addresses).
 */
#define _tscroll1() __asm__ __volatile__ ( \
        "pushf \n\t" \
        "push %%es \n\t" \
        "push %%ds \n\t" \
        "cld \n\t" \
        "mov $0x0038,%%ax \n\t" \
        "cli \n\t" \
        "mov %%ax,%%es \n\t" \
        "mov %%ax,%%ds \n\t" \
        "mov $0x00000000,%%edi \n\t" \
        "mov $0x000000a0,%%esi \n\t" \
        "mov $0x00000f00,%%ecx \n\t" \
        "rep \n\t" \
        "movsb \n\t" \
        "mov $0x00000f00,%%edi \n\t" \
        "mov $0x00000050,%%ecx \n\t" \
        "mov $0,%%eax \n\t" \
        "_clr_last_line: \n\t" \
        "mov %%al,%%ds:(%%di) \n\t" \
        "add $2,%%di \n\t" \
        "loop _clr_last_line \n\t" \
        "pop %%ds \n\t" \
        "pop %%es \n\t" \
        "popf \n\t" \
        : \
        : \
        : "%ax","%cx","%di","%si" \
        )

void tscroll1(void)
{
  _tscroll1();
}

/* Puts character at an offset in VIDEO_RAM segment (starting at 0x000b8000). */
#define _tputc(offset,c) __asm__ __volatile__ ( \
        "pushf \n\t" \
        "push %%es \n\t" \
        "mov $0x0038,%%ax \n\t" \
        "cli \n\t" \
        "mov %%ax,%%es \n\t" \
        "mov %%bl,%%es:(%%di) \n\t" \
        "pop %%es \n\t" \
        "popf \n\t" \
        : \
        : "D" (offset), "b" (c) \
        : "%ax" \
        )

/*
 * Prints one character.
 */
void tputck(char c)
{
  int offset;
  switch (c)
  {
    case '\n':
      tcurx = 0;
      tcury++;
      break;
    case '\t':
      /* Shift to the nearest tab.  */
      tcurx += TTAB_SIZE - (tcurx % TTAB_SIZE);
      break;
    case '\r':
      /* "Rewind" the line. */
      tcurx = 0;
      break;
    case '\b':
      /* Backspace */
      if (tcurx > 0)
        tcurx--;
      break;
    default:
      /* Calculate offset to 0x000b8000 and put char. */
      offset = 2 * (TMAX_X * tcury + tcurx);
      if (c >= 32) /* Not all characters should be visible on the screen. */
        if ((unsigned int)offset <= 0x00000fff) /* This is the VIDEO_RAM segment limit. So - avoid segentation fault. */
          _tputc(offset, c);
      tcurx++;
  }
  /* Check if overflow: */
  while (tcurx >= TMAX_X)
  {
    tcurx -= TMAX_X;
    tcury++;
  }
  while (tcury >= TMAX_Y)
  {
    /* We have reached the last line in the local terminal. */
    tscroll1();
    tcury--;
  }
}

/*
 * Print as decimal.
 */
void tprintdec(int i)
{
  /* Check if negative */
  if (i < 0)
  {
    tputck('-');
    tprintdec(-i);
  }
  else
  {
    /* Ok, it is not negative. Let's start... */
    /* First, cut off last digit. */
    char last_dig = (char)(i % 10);
    /* Second, use this function recursively to print all but last digits. */
    int first_dig = (i - (int)last_dig) / 10;
    if (first_dig != 0)
      tprintdec(first_dig);
    /* Finally, print last digit. */
    tputck('0' + last_dig);
  }
}

/*
 * Print as decimal. Shift to left (negative align) or rigth (positive align).
 */
int tprintdeca(int i, int align)
{
  /* At least one character is going to be printed. */
  int len = 0;
  if (align != 0)
    align -= sgn(align); /* Since, at least one character is going to be printed, decrement absolute value of the padding parameter. */
  /* Check if negative */
    /* Ok, it is not negative. Let's start... */
    /* First, cut off last digit. */
    int absi = abs(i);
    char last_dig = (char)(absi % 10);
    /* Second, use this function recursively to print all but last digits. */
    int first_dig = (absi - (int)last_dig) / 10;
    if (first_dig != 0)
    {
      len += tprintdeca((i > 0) ? first_dig : -first_dig, (align > 0) ? align : 0); /* Most inner recursive call will align only to the right, never to the left. */
      if (align > 0)
      {
        if (len <= align)
          align -= len;
        else
          align = 0;
      }
      else if (align < 0)
      {
        if (len <= -align)
          align += len;
        else
          align = 0;
      }
    }
    else
    {
      while (align > (i < 0))
      {
        tputck(' ');
        align--;
        len++;
      }
      if (i < 0)
      {
        tputck('-');
        align = 0;
        len++;
      }
    }
    /* Finally, print the last digit. */
    tputck('0' + last_dig);
    while (align < 0)
    {
      tputck(' ');
      align++;
      len++;
    }
  
  return ++len;
}

/*
 * Print text in the local terminal.
 */
void tprintk(char * s)
{
  /* Print chars until terminating null is found: */
  while (*s != 0)
  {
    tputck(*s);
    s++;
  }
}

#define PRNFMT_STATE_NORMAL  0
#define PRNFMT_STATE_ESCAPED 1
#define PRNFMT_STATE_NUMBER0 2


/*
 * Prints formatted string.
 */
void tprintfk(char * fmtstr, ...)
{
  int state = PRNFMT_STATE_NORMAL;
  char * s;
  int minus;
  int fmtnum0;
  u32 h8;
  int iarg;
  int slen;
  va_list arg;
  va_start(arg, fmtstr);
  while (*fmtstr != 0) /* Null terminates the string. */
  {
    switch (state)
    {
      case PRNFMT_STATE_NORMAL:
        if (*fmtstr == '%')
          state = PRNFMT_STATE_ESCAPED;
        else
          tputck(*fmtstr);
        break;
      case PRNFMT_STATE_ESCAPED:
        switch (*fmtstr)
        {
          case '-':
            minus = 1;
            fmtnum0 = 0;
            state = PRNFMT_STATE_NUMBER0;
            break;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            minus = 0;
            fmtnum0 = (int)(*fmtstr) - '0';
            state = PRNFMT_STATE_NUMBER0;
            break;
          case 's':
            s = va_arg(arg, char *);
            tprintk(s);
            state = PRNFMT_STATE_NORMAL;
            break;
          case 'd':
          case 'i':
            iarg = va_arg(arg, int);
            tprintdec(iarg);
            state = PRNFMT_STATE_NORMAL;
            break;
          case 'x':
          case 'X':
            h8 = va_arg(arg, u32);
            tprinthexl(h8);
            state = PRNFMT_STATE_NORMAL;
            break;
          case '%':
            tprintk("%");
            state = PRNFMT_STATE_NORMAL;
            break;
          default:
            state = PRNFMT_STATE_NORMAL; /* If unknown, let's get back to "normal" state. */
        }
        break;
      case PRNFMT_STATE_NUMBER0:
        switch (*fmtstr)
        {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            fmtnum0 = (fmtnum0 * 10) + (int)(*fmtstr) - '0';
            break;
          case 's':
            s = va_arg(arg, char *);
            slen = strlen(s);
            if (minus)
              tprintk(s);
            for (int i = slen; i < fmtnum0; i++)
              tprintk(" ");
            if (!minus)
              tprintk(s);
            state = PRNFMT_STATE_NORMAL;
            break;
          case 'd':
          case 'i':
            iarg = va_arg(arg, int);
            tprintdeca(iarg, minus ? -fmtnum0 : fmtnum0);
            state = PRNFMT_STATE_NORMAL;
            break;
          case 'x':
          case 'X':
            h8 = va_arg(arg, u32);
            if (fmtnum0 <= 2)
            {
              if (minus)
                tprinthexb(h8);
              for (int i = 2; i < fmtnum0; i++)
                tprintk(" ");
              if (!minus)
                tprinthexb(h8);
            }
            else if (fmtnum0 <= 4)
            {
              if (minus)
                tprinthexw(h8);
              for (int i = 4; i < fmtnum0; i++)
                tprintk(" ");
              if (!minus)
                tprinthexw(h8);
            }
            else
            {
              if (minus)
                tprinthexl(h8);
              for (int i = 8; i < fmtnum0; i++)
                tprintk(" ");
              if (!minus)
                tprinthexl(h8);
            }
            state = PRNFMT_STATE_NORMAL;
            break;
          default:
            state = PRNFMT_STATE_NORMAL; /* If unknown, let's get back to "normal" state. */
        }
        break;
    }
    fmtstr++; /* Next char. */
  }
  va_end(arg);
}


/*
 * Move cursor to (x,y).
 * Coordinates are 0-based.
 */
void tmvcur(int x, int y)
{
  if ((x >= 0) && (x < TMAX_X) && (y >= 0) && (y < TMAX_Y))
  {
    tcurx = x;
    tcury = y;
  }
}

/*
 * Prints byte as hex.
 */
void tprinthexb(u8 b)
{
  char s[3];
  unsigned char nipple;
  nipple = (b & 0xf0) >> 4;
  s[0] = ((nipple <= 9) ? ('0' + nipple) : ('a' + nipple - 10));
  nipple = b & 0x0f;
  s[1] = ((nipple <= 9) ? ('0' + nipple) : ('a' + nipple - 10));
  s[2] = '\000';
  tprintk(s);
}

/*
 * Prints word as hex.
 */
void tprinthexw(u16 w)
{
  tprinthexb((u8)((w >> 8) & 0xff));
  tprinthexb((u8)(w & 0xff));
}

/*
 * Prints long as hex.
 */
void tprinthexl(u32 l)
{
  tprinthexw((u16)((l >> 16) & 0xffff));
  tprinthexw((u16)(l & 0xffff));
}

#define tprinthexptr(ptr) tprinthexl(ptr)


#endif
