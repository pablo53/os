/*
 * (C) 2011 Pawel Ryszawa
 *  System timing start-up routines.
 */

#ifndef _MAIN_TIME_H
#define _MAIN_TIME_H 1


#include "../asm/types.h"

#ifdef CONFIG_PIT_8253

void init_8253(void);

#endif

u32 get_ticks(void); /* Returns current ticks - value incrementeb by the system timer. */
int is_before_ticks(u32); /* Compares current time (expressed in ticks) to the given value. */
u32 count_ticks_limit(int); /* Returns a "ticks" value from the future. */
u32 count_ticks_limit_u(int); /* Returns a "ticks" value from the future. */
void loop_interval(int); /* Make a "busy wait" loop for a while. */
void loop_interval_u(int); /* Make a "busy wait" loop for a while. */

/* Kernel's timer services: */

typedef int (*trg_t)(void *); /* Trigger returns 0 if it does not want to be fired any more. */

typedef struct
{
  /* Fields copied from the setting timer routine: */
  trg_t trg; /* Trigger (that returns 0 in the last cycle). */
  void * prv_dat; /* Trigger's private data passed to it at fire time. */
  unsigned int intvl; /* Interval in ms (or "absolutely not less before, but ASAP").  */
  /* "Hidden" fields (maintained automatically): */
  unsigned int intvl_ticks; /* Interval computed in interrupt ticks. */
  unsigned int cntdn_ticks; /* Ticks to the nearest firing (count-down). */
} ktimer_t;

int set_ktimer(trg_t, void *, unsigned int);


#endif

