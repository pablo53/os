/*
 * (C) 2011 Pawel Ryszawa
 *
 * Panic & halting routines.
 */

#ifndef _MAIN_SYSTEM_C
#define _MAIN_SYSTEM_C 1


#include "../globals.h"

#include "system.h"
#include "../asm/i386.h"
#include "../asm/types.h"
#include "lterm.h"

#ifdef CONFIG_MULTIPROCESSOR

//TODO

#else

/*
 * Halt the whole system.
 */
void ksystem_halt(void)
{
  tprintk("System down.\n");
  cli();
  while (1)
    hlt();
}

#endif

/*
 * Kernel panic.
 */
void kpanic(void)
{
  tprintk("Kernel panic!\n");
  ksystem_halt();
}

/*
 * Checks whether interrupts are not masked.
 */
int test_int_on(void)
{
  u32 flags;
  
  /* Check IF flag. */
  get_flags(flags);
  
  /* Return non-0 if IF flag is set. */
  return flags & X86_FLAG_IF;
}

/*
 * Halts processor if interrupts are enabled (not masked).
 * This can be considered as a safe version of HLT instruction.
 */
void soft_halt(void)
{
  if (test_int_on())
    hlt();
}


#endif
