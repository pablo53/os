/*
 * (C) 2011 Pawel Ryszawa
 *
 */


#ifndef _MAIN_TIME_C
#define _MAIN_TIME_C 1


#include "../globals.h"

#include "time.h"

#include "../asm/i386.h"
#include "intr.h"

#ifdef CONFIG_PIT_8253

#define PIT8253_CH0_PORT_TIMER0  0x40
#define PIT8253_CH1_PORT_TIMER0  0x41
#define PIT8253_CH2_PORT_TIMER0  0x42
#define PIT8253_CTL_PORT_TIMER0  0x43
#define PIT8253_CH0_PORT_TIMER1  0x50
#define PIT8253_CH1_PORT_TIMER1  0x51
#define PIT8253_CH2_PORT_TIMER1  0x52
#define PIT8253_CTL_PORT_TIMER1  0x53

#define PIT8253_CTL_CH0     0x00
#define PIT8253_CTL_CH1     0x40
#define PIT8253_CTL_CH2     0x80

#define PIT8253_CTL_RW_LO   0x10
#define PIT8253_CTL_RW_HI   0x20
#define PIT8253_CTL_RW_DBL  0x30

#define PIT8253_CTL_MODE0   0x00
#define PIT8253_CTL_MODE1   0x02
#define PIT8253_CTL_MODE2   0x04
#define PIT8253_CTL_MODE3   0x06
#define PIT8253_CTL_MODE4   0x08
#define PIT8253_CTL_MODE5   0x0a

#define PIT8253_CTL_CNT_BIN 0x00
#define PIT8253_CTL_CNT_BCD 0x01

#define HZ       1193181
#define FREQ_DIV 0xfff0
#define FREQ (HZ / FREQ_DIV)

#define INIT_8253_TIMER0_CH0(freq_div) \
  outb(PIT8253_CTL_PORT_TIMER0, PIT8253_CTL_CH0 | PIT8253_CTL_RW_DBL | PIT8253_CTL_MODE2 | PIT8253_CTL_CNT_BIN); \
  outb(PIT8253_CH0_PORT_TIMER0, (char)(freq_div & 0x00ff)); \
  outb(PIT8253_CH0_PORT_TIMER0, (char)((freq_div & 0xff00) >> 8))

/*
 * PIT initialization routine.
 * For now, we focus on channel #0 only, thus leaving
 * channels #1 and #2 intact.
 */
void init_8253(void)
{
  INIT_8253_TIMER0_CH0(FREQ_DIV);
}


#endif

/* Number of tick since the system start-up: */
__asm__ (
 "_ticks:\n\t"
 ".long 0"
);

/*
 * Gets current ticks.
 */
u32 get_ticks(void)
{
  int ticks;
  __asm__ __volatile__ (
    "lock\n\t"
    "mov _ticks,%0"
    : "=r" (ticks)
    :
  );
  return ticks;
}

/*
 * Increment ticks by 1.
 */
static void tick(void)
{
  __asm__ __volatile__ ("lock\n\t" "incl _ticks" : : );
}

/*
 * How many ticks must pass by so as to "msec" milliseconds have passed by as well.
 */
static u32 sec_to_ticks(int sec)
{
  return ((sec * HZ) / FREQ_DIV);
}

/*
 * Assess "ticks" value after "msec" milliseconds since now.
 */
u32 count_ticks_limit(int msec)
{
  u32 interval = sec_to_ticks(msec);
  return get_ticks() + (interval / 1000) + ((interval % 1000) ? 1 : 0); /* Round to ceil. */
}

/*
 * Assess "ticks" value after "usec" microseconds since now.
 */
u32 count_ticks_limit_u(int usec)
{
  u32 interval = sec_to_ticks(usec);
  return get_ticks() + (interval / 1000000) + ((interval % 1000000) ? 1 : 0); /* Round to ceil. */
}

/*
 * Check whether the time is before the given ticks.
 */
int is_before_ticks(u32 t)
{
  return ((((int)get_ticks()) - ((int)t)) > 0);
}

/*
 * Loop for at least the given time period (in milliseconds).
 */
void loop_interval(int msec)
{
  u32 limit = count_ticks_limit(msec);
  while (is_before_ticks(limit))
    ;
}

/*
 * Loop for at least the given time period (in microseconds).
 */
void loop_interval_u(int usec)
{
  u32 limit = count_ticks_limit_u(usec);
  while (is_before_ticks(limit))
    ;
}


#endif
